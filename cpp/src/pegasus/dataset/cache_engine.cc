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

LruCacheEngine::LruCacheEngine(long capacity): cache_(capacity), cache_store_manager_(new CacheStoreManager()) {}

 Status LruCacheEngine::PutValue(std::string partition_path, int column_id, std::shared_ptr<CacheRegion> cache_region) {
  CacheEntryKey key = CacheEntryKey(partition_path, column_id);
  cache_.insert(key, cache_region);
 }

LruCacheEngine::~LruCacheEngine() {}

NonEvictionCacheEngine::NonEvictionCacheEngine() {}
NonEvictionCacheEngine:: ~NonEvictionCacheEngine(){}

Status NonEvictionCacheEngine::PutValue(std::string partition_path, int column_id, std::shared_ptr<CacheRegion> cache_region) {
 }

} // namespace pegasus