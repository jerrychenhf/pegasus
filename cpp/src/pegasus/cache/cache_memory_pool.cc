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

namespace pegasus {
    
CacheMemoryPool::CacheMemoryPool(std::shared_ptr<CacheEngine> cache_engine) {
  cache_engine_ = cache_engine;
  occupied_size = 0;
}
CacheMemoryPool::~CacheMemoryPool() {}

arrow::Status CacheMemoryPool::Allocate(int64_t size, uint8_t** out) {
  if (size < 0) {
    return arrow::Status::Invalid("negative malloc size");
  }
  LruCacheEngine *lru_cache_engine = dynamic_cast<LruCacheEngine *>(cache_engine_.get());
  std::shared_ptr<CacheStore> cache_store;
  lru_cache_engine->cache_store_manager_->GetCacheStore(&cache_store);
  std::shared_ptr<CacheRegion> cache_region;
  cache_store->Allocate(size, &cache_region);
  out = cache_region->address();
  occupied_size = size + occupied_size;
  return arrow::Status::OK();
}

void CacheMemoryPool::Free(uint8_t* buffer, int64_t size)  { std::free(buffer); }

arrow::Status CacheMemoryPool::Reallocate(int64_t old_size, int64_t new_size, uint8_t** ptr) {
  *ptr = reinterpret_cast<uint8_t*>(std::realloc(*ptr, new_size));

  if (*ptr == NULL) {
    return arrow::Status::OutOfMemory("realloc of size ", new_size, " failed");
  }

  return arrow::Status::OK();
}

int64_t CacheMemoryPool::bytes_allocated() const  { return occupied_size; }
  
int64_t CacheMemoryPool::max_memory() const {return 100;}

std::string CacheMemoryPool::backend_name() const { return "DRAM"; }

} // namespace pegasus