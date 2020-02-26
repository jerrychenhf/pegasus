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

using namespace std;

namespace pegasus {

LruCacheEngine::LruCacheEngine(int64_t capacity)
  : cache_store_manager_(new CacheStoreManager()) {
  lru_cache_ = new LRUCache(capacity);
}

Status LruCacheEngine::Init() {
  RETURN_IF_ERROR(cache_store_manager_->Init());
  return Status::OK();
}

Status LruCacheEngine::PutValue(LRUCache::CacheKey key) {
  LRUCache::PendingEntry pending_entry = lru_cache_->Allocate(key, sizeof(key));

  LRUCacheHandle inserted_handle;
  lru_cache_->Insert(&pending_entry, &inserted_handle);

  if (!pending_entry.valid() && inserted_handle.valid()) {
    return Status::OK();
  } else {
    return Status::Invalid("Failed to insert the cached colmn into lru cache");
  }
}

} // namespace pegasus