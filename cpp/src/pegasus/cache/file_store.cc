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

#include "cache/file_store.h"
#include "ipc/allocator.h"
#include "cache/store_manager.h"
#include "ipc/malloc.h"
#include "ipc/store_config.h"

DECLARE_int32(store_file_capacity_gb);

namespace pegasus {
constexpr int64_t kBlockSize = 64;

FileStore::FileStore(int64_t capacity)
  : capacity_(capacity),
    used_size_(0) {
}

Status FileStore::Init(const std::unordered_map<string, string>* properties) {
  LOG(INFO) << "Init the binary store";
  // Get the file store path from properties
  auto entry  = properties->find(StoreManager::STORE_PROPERTY_PATH);
  
  std::string file_store_path = "";

  if (entry == properties->end()) {
    file_store_path = "/dev/shm";
  } else {
    file_store_path = entry->second;
  }
  LOG(INFO) << "The file store path is " << file_store_path;
  
  // Init the store config.
  StoreConfig* file_store_config = new StoreConfig();
  file_store_config->hugepages_enabled= false;
  file_store_config->directory = file_store_path;
  store_config = file_store_config;

  int64_t capacity = ((int64_t) FLAGS_store_file_capacity_gb) * StoreManager::GIGABYTE;

  Allocator::SetFootprintLimit(static_cast<size_t>(capacity));
  
  // We are using a single memory-mapped file by mallocing and freeing a single
  // large amount of space up front. According to the documentation,
  // dlmalloc might need up to 128*sizeof(size_t) bytes for internal
  // bookkeeping.
  void* pointer = Allocator::Memalign(
        kBlockSize, Allocator::GetFootprintLimit() - 256 * sizeof(size_t));
  DCHECK(pointer != nullptr);

  // This will unmap the file, but the next one created will be as large
  // as this one (this is an implementation detail of dlmalloc). 
  Allocator::Free(pointer, Allocator::GetFootprintLimit() - 256 * sizeof(size_t));
  return Status::OK();
}

Status FileStore::Reallocate(int64_t old_size, int64_t new_size, StoreRegion* store_region) {
  return Status::OK();
}

Status FileStore::Allocate(int64_t size, StoreRegion* store_region) {
  DCHECK(store_region != NULL);
  
  //check the free size. If no free size available, fail
  if (size > (capacity_ - used_size_)) {
    stringstream ss;
    ss << "Allocate failed in file memory store when the available size < allocated size. The allocated size: "
     << size << ". The available size: " << (capacity_ - used_size_);
    LOG(ERROR) << ss.str();
    return Status::Invalid("Request memory size" , size, "is larger than available size.");
  }

  uint8_t* pointer = nullptr;

  pointer = reinterpret_cast<uint8_t*>(Allocator::Memalign(kBlockSize, size));

  if (pointer == nullptr) {
    stringstream ss;
    ss << "Allocate failed with OOM in file store. The allocated size:"
     << size << ". The available size: " << (capacity_ - used_size_);
    LOG(ERROR) << ss.str();
    return Status::OutOfMemory("malloc of size ", size, " failed");
  }

  // get the fd, map size offset info by the address.
  // GetMallocMapinfo(pointer, &fd, &map_size, &offset);

  store_region->reset_address(pointer, size, size);

  used_size_ += size;
  LOG(INFO) << "Successfully allocated in memory store. And the allocated size is " << size;
  return Status::OK();
}

Status FileStore::Free(StoreRegion* store_region) {
  
  DCHECK(store_region != NULL);

  used_size_ -= store_region->occupies_size();
 
  Allocator::Free(store_region->address(), static_cast<size_t>(store_region->occupies_size()));

  return Status::OK();
}

int64_t FileStore::GetFreeSize() {
  return capacity_ - used_size_; 
}

int64_t FileStore::GetUsedSize() {
  return used_size_;
}

std::string FileStore::GetStoreName() {
    return "BINARY";
}

} // namespace pegasus
