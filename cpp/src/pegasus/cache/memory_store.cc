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

#include "cache/memory_store.h"

namespace pegasus {

MemoryStore::MemoryStore(int64_t capacity)
  : capacity_(capacity),
    used_size_(0) {
}

Status MemoryStore::Init(const std::unordered_map<string, string>* properties) {
  LOG(INFO) << "Init the memory store";
  return Status::OK();
}

Status MemoryStore::Allocate(int64_t size, StoreRegion* store_region) {
  DCHECK(store_region != NULL);
  
  //check the free size. If no free size available, fail
  if (size > (capacity_ - used_size_)) {
    stringstream ss;
    ss << "Allocate failed in memory store when the available size < allocated size. The allocated size: "
     << size << ". The available size: " << (capacity_ - used_size_);
    LOG(ERROR) << ss.str();
    return Status::Invalid("Request memory size" , size, "is larger than available size.");
  }
  
  uint8_t* out;
  const int result = posix_memalign(reinterpret_cast<void**>(&out), 64,
                                      static_cast<size_t>(size));
  if (result == ENOMEM) {
    stringstream ss;
    ss << "Allocate failed with OOM in memory store after call posix_memalign method. The allocated size:"
     << size << ". The available size: " << (capacity_ - used_size_);
    LOG(ERROR) << ss.str();
    return Status::OutOfMemory("malloc of size ", size, " failed");
   }

  if (result == EINVAL) {
    stringstream ss;
    ss << "Allocate failed with invalid alignment parameter in memory store after call posix_memalign method. The allocated size:"
     << size << ". The available size: " << (capacity_ - used_size_);
    LOG(ERROR) << ss.str();
    return Status::Invalid("invalid alignment parameter: ", 64);
  }

  store_region->reset_address(out, size, size);
  used_size_ += size;
  LOG(INFO) << "Successfully allocated in memory store. And the allocated size is " << size;
  return Status::OK();
}

Status MemoryStore::Reallocate(int64_t old_size, int64_t new_size, StoreRegion* store_region) {
  DCHECK(store_region != NULL);

  //check the free size. If no free size available, fail
  if (new_size > (capacity_ - used_size_)) {
    stringstream ss;
    ss << "Reallocate failed in memory store when the available size < new allocated size. The new allocated size: "
     << new_size << ". The available size: " << (capacity_ - used_size_);
    LOG(ERROR) << ss.str();
    return Status::Invalid("Request memory size" , (new_size - old_size), "is larger than available size.");
  }

  uint8_t* out = nullptr;
  const int result = posix_memalign(reinterpret_cast<void**>(&out), 64,
                                      static_cast<size_t>(new_size));
  if (result == ENOMEM) {
    stringstream ss;
    ss << "Reallocate failed with OOM in memory store after call posix_memalign method. The new allocated size:"
     << new_size << ". The available size: " << (capacity_ - used_size_);
    LOG(ERROR) << ss.str();
    return Status::OutOfMemory("malloc of size ", new_size, " failed");
   }

  if (result == EINVAL) {
    stringstream ss;
    ss << "Reallocate failed with invalid alignment parameter in memory store after call posix_memalign method. The new allocated size:"
     << new_size << ". The available size: " << (capacity_ - used_size_);
    LOG(ERROR) << ss.str();
    return Status::Invalid("invalid alignment parameter: ", 64);
  }

  uint8_t* old_address = store_region->address();
  memcpy(out, old_address, static_cast<size_t>(std::min(new_size, old_size)));
  free(old_address);

  store_region->reset_address(out, new_size, new_size);
  used_size_ += (new_size - old_size);
  LOG(INFO) << "Successfully reallocated in dcpmm store. And the reallocated size is " << new_size;
  return Status::OK();
}

Status MemoryStore::Free(StoreRegion* store_region) {
  DCHECK(store_region != NULL);

  used_size_ -= store_region->occupies_size();
  
  std::free(store_region->address());
  
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

} // namespace pegasus