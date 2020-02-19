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

#include "cache/cache_memory_pool.h"
#include "common/logging.h"

namespace pegasus {
    
CacheMemoryPool::CacheMemoryPool(std::shared_ptr<CacheEngine> cache_engine)
  : cache_engine_(cache_engine), occupied_size(0) {
}
CacheMemoryPool::~CacheMemoryPool() {}

Status CacheMemoryPool::Create() {
  RETURN_IF_ERROR(cache_engine_->GetCacheStore(&cache_store_));
  return Status::OK();
}

Status CacheMemoryPool::GetCacheRegion(int64_t size, StoreRegion* store_region) {
  if (cache_store_ == nullptr)
    return Status::Invalid("Cache store is not correctly initialized.");
  
  RETURN_IF_ERROR(cache_store_->Allocate(size, store_region));
  return Status::OK();
}

arrow::Status CacheMemoryPool::Allocate(int64_t size, uint8_t** out) {
  if (size < 0) {
    return arrow::Status::Invalid("negative malloc size");
  }
  
  StoreRegion store_region;
  Status status = GetCacheRegion(size, &store_region);
  if(!status.ok()) {
    return arrow::Status::OutOfMemory("Failed to allocate cache region in cache memory pool");
  }
  
  *out = reinterpret_cast<uint8_t*>(store_region.address());
  occupied_size += size;
  return arrow::Status::OK();
}

void CacheMemoryPool::Free(uint8_t* buffer, int64_t size)  {
  if (cache_store_ == nullptr)
    return;
    
  CacheRegion cacheRegion(buffer, size, size);
  cache_store_->Free(&cacheRegion);
  occupied_size -= size;
}

arrow::Status CacheMemoryPool::Reallocate(int64_t old_size, int64_t new_size, uint8_t** ptr) {
  *ptr = reinterpret_cast<uint8_t*>(std::realloc(*ptr, new_size));

  if (*ptr == nullptr) {
    return arrow::Status::OutOfMemory("realloc of size ", new_size, " failed");
  }

  occupied_size += new_size;

  return arrow::Status::OK();
}

int64_t CacheMemoryPool::bytes_allocated() const  {
  return occupied_size;
}
  
int64_t CacheMemoryPool::max_memory() const {
  return cache_store_->GetCapacity();
}

std::string CacheMemoryPool::backend_name() const {
  if (cache_store_ != nullptr) {
    return cache_store_->GetStore()->GetStoreName();
  } else {
    return "MEMORY";
  } 
}

} // namespace pegasus