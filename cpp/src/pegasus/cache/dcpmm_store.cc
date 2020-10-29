// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#include "cache/dcpmm_store.h"

#include <dlfcn.h>
#include <mutex>
#include <glog/logging.h>
#include "gutil/strings/substitute.h"
#include "util/scoped_cleanup.h"
#include "util/flag_tags.h"
#include "cache/store_manager.h"

#ifndef MEMKIND_PMEM_MIN_SIZE
  #define MEMKIND_PMEM_MIN_SIZE (1024 * 1024 * 16) // Taken from memkind 1.9.0.
#endif

DEFINE_bool(dcpmm_cache_simulate_allocation_failure, false,
            "If true, the dcpmm cache will inject failures in calls to memkind_malloc "
            "for testing.");
TAG_FLAG(dcpmm_cache_simulate_allocation_failure, unsafe);

DECLARE_int32(store_dcpmm_initial_capacity_gb);

namespace pegasus {

// Taken together, these typedefs and this macro make it easy to call a
// memkind function:
//
//  CALL_MEMKIND(memkind_malloc, vmp_, size);
typedef int (*memkind_create_pmem)(const char*, size_t, memkind**);
typedef int (*memkind_destroy_kind)(memkind*);
typedef void* (*memkind_malloc)(memkind*, size_t);
typedef void* (*memkind_realloc)(memkind*, void*, size_t);
typedef size_t (*memkind_malloc_usable_size)(memkind*, void*);
typedef void (*memkind_free)(memkind*, void*);
typedef int (*memkind_posix_memalign)(memkind*, void**, size_t, size_t);
#define CALL_MEMKIND(func_name, ...) ((func_name)g_##func_name)(__VA_ARGS__)

// Function pointers into memkind; set by InitMemkindOps().
void* g_memkind_create_pmem;
void* g_memkind_destroy_kind;
void* g_memkind_malloc;
void* g_memkind_realloc;
void* g_memkind_malloc_usable_size;
void* g_memkind_posix_memalign;
void* g_memkind_free;

// After InitMemkindOps() is called, true if memkind is available and safe
// to use, false otherwise.
bool g_memkind_available;

std::once_flag g_memkind_ops_flag;

// Try to dlsym() a particular symbol from 'handle', storing the result in 'ptr'
// if successful.
Status TryDlsym(void* handle, const char* sym, void** ptr) {
  dlerror(); // Need to clear any existing error first.
  void* ret = dlsym(handle, sym);
  char* error = dlerror();
  if (error) {
    return Status::Invalid(error);
  }
  *ptr = ret;
  return Status::OK();
}

// Try to dlopen() memkind and set up all the function pointers we need from it.
//
// Note: in terms of protecting ourselves against changes in memkind, we'll
// notice (and fail) if a symbol is missing, but not if it's signature has
// changed or if there's some subtle behavioral change. A scan of the memkind
// repo suggests that backwards compatibility is enforced: symbols are only
// added and behavioral changes are effected via the introduction of new symbols.
void InitMemkindOps() {
  g_memkind_available = false;

  // Use RTLD_NOW so that if any of memkind's dependencies aren't satisfied
  // (e.g. libnuma is too old and is missing symbols), we'll know up front
  // instead of during cache operations.
  void* memkind_lib = dlopen("libmemkind.so.0", RTLD_NOW);
  if (!memkind_lib) {
    LOG(WARNING) << "could not dlopen: " << dlerror();
    return;
  }
  auto cleanup = MakeScopedCleanup([&]() {
    dlclose(memkind_lib);
  });

#define DLSYM_OR_RETURN(func_name, handle) do { \
    const Status _s = TryDlsym(memkind_lib, func_name, handle); \
    if (!_s.ok()) { \
      LOG(WARNING) << _s.ToString(); \
      return; \
    } \
  } while (0)

  DLSYM_OR_RETURN("memkind_create_pmem", &g_memkind_create_pmem);
  DLSYM_OR_RETURN("memkind_destroy_kind", &g_memkind_destroy_kind);
  DLSYM_OR_RETURN("memkind_malloc", &g_memkind_malloc);
  DLSYM_OR_RETURN("memkind_posix_memalign", &g_memkind_posix_memalign);
  DLSYM_OR_RETURN("memkind_malloc_usable_size", &g_memkind_malloc_usable_size);
  DLSYM_OR_RETURN("memkind_free", &g_memkind_free);
#undef DLSYM_OR_RETURN

  g_memkind_available = true;

  // Need to keep the memkind library handle open so our function pointers
  // remain loaded in memory.
  cleanup.cancel();
}

DCPMMStore::DCPMMStore(int64_t capacity)
  : capacity_(capacity),
    used_size_(0),
    vmp_(nullptr) {
}

Status DCPMMStore::Init(const std::unordered_map<string, string>* properties) {
  // Get the dcpmm path from properties
  auto entry  = properties->find(StoreManager::STORE_PROPERTY_PATH);

  if (entry == properties->end()) {
    return Status::Invalid("Need to specific the DCPMM path first.");
  }
  
  std::string dcpmm_path = entry->second;

  // initialize the DCPMM
  std::call_once(g_memkind_ops_flag, InitMemkindOps);

  // TODO(adar): we should plumb the failure up the call stack, but at the time
  // of writing the dcpmm cache is only usable by the block cache, and its use of
  // the singleton pattern prevents the surfacing of errors.
  CHECK(g_memkind_available) << "Memkind not available!";

  int64_t initial_capacity = ((int64_t) FLAGS_store_dcpmm_initial_capacity_gb) * StoreManager::GIGABYTE;

  // memkind_create_pmem() will fail if the capacity is too small, but with
  // an inscrutable error. So, we'll check ourselves.
  CHECK_GE(initial_capacity, MEMKIND_PMEM_MIN_SIZE)
    << "configured capacity " << capacity_ << " bytes is less than "
    << "the minimum capacity for an dcpmm cache: " << MEMKIND_PMEM_MIN_SIZE;
  
  int err = CALL_MEMKIND(memkind_create_pmem, dcpmm_path.c_str(), initial_capacity, &vmp_);
  
  // If we cannot create the cache pool we should not retry.
  PLOG_IF(FATAL, err) << "Could not initialize DCPMM cache library in path "
                           << dcpmm_path.c_str();
  LOG(INFO) << "Successfully init dcpmm store. And the initial capacity is " << initial_capacity;
  return Status::OK();
}

Status DCPMMStore::Allocate(int64_t size, StoreRegion* store_region) {
  DCHECK(store_region != NULL);

  if (size > (capacity_ - used_size_)) {
    stringstream ss;
    ss << "Allocate failed in DCPMM store when the available size < allocated size. The allocated size: "
     << size << ". The available size: " << (capacity_ - used_size_);
    LOG(ERROR) << ss.str();
    return Status::Invalid("Request dcpmm size" , size, "is larger than available size.");
  }

 // void* p = CALL_MEMKIND(memkind_malloc, vmp_, size);

  void* p;
  int err = CALL_MEMKIND(memkind_posix_memalign, vmp_, &p, 64, size);

  if (err) {
    stringstream ss;
    ss << "Allocate failed in DCPMM store after call memkind_posix_memalign method. The allocated size:"
     << size << ". The available size: " << (capacity_ - used_size_);
    LOG(ERROR) << ss.str();
    return Status::OutOfMemory("Allocate of size ", size, " failed in DCPMM store");
  }

  size_t occupied_size = CALL_MEMKIND(memkind_malloc_usable_size, vmp_, p);

  if (occupied_size < static_cast<size_t>(size)) {
    stringstream ss;
    ss << "Allocate failed in DCPMM store when the occupied size < allocated size. The allocated size: "
     << size << ". The occupied size: " << occupied_size;
    LOG(ERROR) << ss.str();
    return Status::OutOfMemory("Allocate of size ", size, " failed in DCPMM store");
  }

  uint8_t* address = reinterpret_cast<uint8_t*>(p);
  store_region->reset_address(address, size, occupied_size);
  used_size_ += occupied_size;
  LOG(INFO) << "Successfully allocated in dcpmm store. And the allocated size is " << size;
  return Status::OK();
}

Status DCPMMStore::Reallocate(int64_t old_size, int64_t new_size, StoreRegion* store_region) {
  DCHECK(store_region != NULL);

  //check the free size. If no free size available, fail
  if (new_size > (capacity_ - used_size_)) {
    stringstream ss;
    ss << "Reallocate failed in DCPMM store when the available size < allocated size. The new allocated size: "
     << new_size << ". The available size: " << (capacity_ - used_size_);
    LOG(ERROR) << ss.str();
    return Status::Invalid("Request memory size" , new_size, "is larger than available size.");
  }
  
  void* old_ptr = reinterpret_cast<void*>(store_region->address());

  void* new_ptr;
  int err = CALL_MEMKIND(memkind_posix_memalign, vmp_, &new_ptr, 64, new_size);

  if (err) {
    stringstream ss;
    ss << "Reallocate failed in DCPMM store after call memkind_posix_memalign method. The new allocated size:"
     << new_size << ". The available size: " << (capacity_ - used_size_);
    LOG(ERROR) << ss.str();
    return Status::OutOfMemory("Reallocate of size ", new_size, " failed in DCPMM store");
  }

  memcpy(new_ptr, old_ptr, static_cast<size_t>(std::min(new_size, old_size)));
  
  // free the old address
  CALL_MEMKIND(memkind_free, vmp_, old_ptr);

  size_t occupied_size = CALL_MEMKIND(memkind_malloc_usable_size, vmp_, new_ptr);

  if (occupied_size < static_cast<size_t>(new_size)) {
    stringstream ss;
    ss << "Reallocate failed in DCPMM store when the occupied size < allocated size. The allocated size: "
     << new_size << ". The occupied size: " << occupied_size;
    LOG(ERROR) << ss.str();
    return Status::OutOfMemory("Reallocate of size ", new_size, " failed in DCPMM store");
  }

  uint8_t* new_address = reinterpret_cast<uint8_t*>(new_ptr);
  store_region->reset_address(new_address, new_size, occupied_size);
  used_size_ += (occupied_size - store_region->occupies_size());

  LOG(INFO) << "Successfully reallocated in dcpmm store. And the reallocated size is " << new_size;
  return Status::OK();
}

Status DCPMMStore::Free(StoreRegion* store_region) {
  DCHECK(store_region != NULL);
  
  used_size_ -= store_region->occupies_size();
  
  void* ptr = reinterpret_cast<void*>(store_region->address());
  CALL_MEMKIND(memkind_free, vmp_, ptr);

  return Status::OK();
}

int64_t DCPMMStore::GetFreeSize() {
  return capacity_ - used_size_;
}

int64_t DCPMMStore::GetUsedSize() {
  return used_size_;
}

std::string DCPMMStore::GetStoreName() {
    return "DCPMM";
}

} // namespace pegasus
