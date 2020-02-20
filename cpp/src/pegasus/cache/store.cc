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

namespace pegasus {

MemoryStore::MemoryStore(int64_t capacity)
  : capacity_(capacity),
    free_size_(capacity),
    used_size_(0) {
}

Status MemoryStore::Init() {
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
  free_size_ -= size;
  used_size_ += size;
  return Status::OK();
}

Status MemoryStore::Free(StoreRegion* store_region) {
  DCHECK(store_region != NULL);
  
  std::free(store_region->address());
  
  free_size_  += store_region->length();
  used_size_ -= store_region->length();
  return Status::OK();
}

int64_t MemoryStore::GetFreeSize() {
  return free_size_; 
}

int64_t MemoryStore::GetUsedSize() {
  return used_size_;
}

std::string MemoryStore::GetStoreName() {
    return "MEMORY";
}

DCPMMStore::DCPMMStore(int64_t capacity)
  : capacity_(capacity),
    free_size_(capacity),
    used_size_(0) {
}

Status DCPMMStore::Init() {
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
  return free_size_;
}

int64_t DCPMMStore::GetUsedSize() {
  return used_size_;
}

std::string DCPMMStore::GetStoreName() {
    return "DCPMM";
}

} // namespace pegasus