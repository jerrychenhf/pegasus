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

#include "dataset/cache_engine.h"

using namespace std;

namespace pegasus {

LruCacheEngine::LruCacheEngine(int64_t capacity)
  : cache_store_manager_(new CacheStoreManager()), cache_(capacity) {
}

Status LruCacheEngine::Init() {
  RETURN_IF_ERROR(cache_store_manager_->Init());
  return Status::OK();
}

Status LruCacheEngine::PutValue(std::string partition_path, int column_id,
   CacheRegion* cache_region, StoreRegion* store_region,
    CacheStore* cache_store) {
  CacheEntryKey key = CacheEntryKey(partition_path, column_id);
  CacheEntryValue* value = new CacheEntryValue(cache_region, store_region, cache_store);
  cache_.insert(key, value);
  return Status::OK();
}


} // namespace pegasus