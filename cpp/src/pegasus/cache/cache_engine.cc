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

#include "cache/cache_engine.h"
#include "runtime/worker_config.h"
#include "cache/cache_store_manager.h"

using namespace std;

namespace pegasus {

LruCacheEngine::LruCacheEngine(int64_t capacity)
  : cache_store_manager_(new CacheStoreManager()) {
  lru_cache_ = new LRUCache(capacity);
}

Status LruCacheEngine::Init(const std::shared_ptr<CacheEngineInfo>& info) {
  RETURN_IF_ERROR(cache_store_manager_->Init(info->cache_stores()));
  RETURN_IF_ERROR(lru_cache_->Init());
  return Status::OK();
}

Status LruCacheEngine::GetCacheStore(CacheStore** cache_store) {
  return cache_store_manager_->GetCacheStore(cache_store);
}
  
Status LruCacheEngine::PutValue(LRUCache::CacheKey* key, int64_t column_size) {
 
  lru_cache_->Insert(key, column_size);
  return Status::OK();
}

Status LruCacheEngine::TouchValue(LRUCache::CacheKey* key) {
  
  lru_cache_->Touch(key);
  return Status::OK();
}

Status LruCacheEngine::EraseValue(LRUCache::CacheKey* key) {
  lru_cache_->Erase(key);
  return Status::OK();
}

} // namespace pegasus