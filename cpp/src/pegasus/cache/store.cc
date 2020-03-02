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

#include "cache/store.h"

#include <dlfcn.h>
#include <mutex>
#include "gutil/strings/substitute.h"
#include "util/scoped_cleanup.h"

#ifndef MEMKIND_PMEM_MIN_SIZE
#define MEMKIND_PMEM_MIN_SIZE (1024 * 1024 * 16) // Taken from memkind 1.9.0.
#endif

struct memkind;

namespace pegasus {

// Taken together, these typedefs and this macro make it easy to call a
// memkind function:
//
//  CALL_MEMKIND(memkind_malloc, vmp_, size);
typedef int (*memkind_create_pmem)(const char*, size_t, memkind**);
typedef int (*memkind_destroy_kind)(memkind*);
typedef void* (*memkind_malloc)(memkind*, size_t);
typedef size_t (*memkind_malloc_usable_size)(memkind*, void*);
typedef void (*memkind_free)(memkind*, void*);
#define CALL_MEMKIND(func_name, ...) ((func_name)g_##func_name)(__VA_ARGS__)

// Function pointers into memkind; set by InitMemkindOps().
void* g_memkind_create_pmem;
void* g_memkind_destroy_kind;
void* g_memkind_malloc;
void* g_memkind_malloc_usable_size;
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
  DLSYM_OR_RETURN("memkind_malloc_usable_size", &g_memkind_malloc_usable_size);
  DLSYM_OR_RETURN("memkind_free", &g_memkind_free);
#undef DLSYM_OR_RETURN

  g_memkind_available = true;

  // Need to keep the memkind library handle open so our function pointers
  // remain loaded in memory.
  cleanup.cancel();
}

MemoryStore::MemoryStore(int64_t capacity)
  : capacity_(capacity),
    used_size_(0) {
}

Status MemoryStore::Init(const std::unordered_map<string, string>* properties) {
  return Status::OK();
}

Status MemoryStore::Allocate(int64_t size, StoreRegion* store_region) {
  DCHECK(store_region != NULL);
  
  //TODO
  //check the free size. If no free size available, fail
  
  uint8_t* address = reinterpret_cast<uint8_t*>(std::malloc(size));
  if (address == NULL) {
    return Status::OutOfMemory("Allocate of size ", size, " failed");
  }
  store_region->reset_address(address, size);
  used_size_ += size;
  return Status::OK();
}

Status MemoryStore::Free(StoreRegion* store_region) {
  DCHECK(store_region != NULL);
  
  std::free(store_region->address());
  
  used_size_ -= store_region->length();
  return Status::OK();
}

int64_t MemoryStore::GetFreeSize() {
  return capacity_ - used_size_; 
}

int64_t MemoryStore::GetUsedSize() {
  return used_size_;
}

std::string MemoryStore::GetStoreName() {
    return "MEMORY";
}

DCPMMStore::DCPMMStore(int64_t capacity)
  : capacity_(capacity),
    used_size_(0) {
}

Status DCPMMStore::Init(const std::unordered_map<string, string>* properties) {
  //TODO
  // initialize the DCPMM
  return Status::OK();
}

Status DCPMMStore::Allocate(int64_t size, StoreRegion* store_region) {
  return Status::OK();
}

Status DCPMMStore::Free(StoreRegion* store_region) {
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