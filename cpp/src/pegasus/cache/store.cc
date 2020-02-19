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

MemoryStore::MemoryStore(int64_t total_size): total_size_(total_size) {

}

Status MemoryStore::Allocate(int64_t size, CacheRegion* cache_region) {
  DCHECK(cache_region != NULL);
  
  uint8_t* address = reinterpret_cast<uint8_t*>(std::malloc(size));
  if (address == NULL) {
    return Status::OutOfMemory("Allocate of size ", size, " failed");
  }
  cache_region->reset_address(address, size);
  total_size_ -= size;
  used_size_ += size;
  return Status::OK();
}

Status MemoryStore::Free(CacheRegion* cache_region) {
  DCHECK(cache_region != NULL);
  
  std::free(cache_region->chunked_array());
  
  total_size_  += cache_region->length();
  used_size_ -= cache_region->length();
  return Status::OK();
}

int64_t MemoryStore::GetTotalSize() {
  return total_size_; 
}

int64_t MemoryStore::GetUsedSize() {
  return used_size_;
}

std::string MemoryStore::GetStoreName() {
    return "MEMORY";
}

DCPMMStore::DCPMMStore(int64_t total_size): total_size_(total_size) {

}

Status DCPMMStore::Allocate(int64_t size, CacheRegion* cache_region) {
  return Status::OK();
}

Status DCPMMStore::Free(CacheRegion* cache_region) {
  return Status::OK();
}

int64_t DCPMMStore::GetTotalSize() {
  return total_size_;
}

int64_t DCPMMStore::GetUsedSize() {
  return used_size_;
}

std::string DCPMMStore::GetStoreName() {
    return "DCPMM";
}

} // namespace pegasus