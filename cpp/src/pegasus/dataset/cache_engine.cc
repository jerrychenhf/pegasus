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

#include "pegasus/dataset/cache_engine.h"
#include "pegasus/runtime/exec_env.h"
#include "pegasus/cache/store_factory.h"

using namespace std;

namespace pegasus {

LruCacheEngine::LruCacheEngine(long capacity): cache_(capacity) {}

 Status LruCacheEngine::PutValue(std::string partition_path, int column_id, CacheEntryHolder cache_entry_holder) {
   // TODO
   // 1. Get the StoreAllocator by calling DatasetCacheStoreManager#GetStoreAllocator method
   // 2. Call StoreManager#Store#Allocate method to allocate the memory to store the value
   // 3. Call the related LRUxxCache.insert to insert value
   // 4. Update the info in DatasetCacheBlockManager
  CacheEntryKey key = CacheEntryKey(partition_path, column_id);
  cache_.insert(key, cache_entry_holder);
 }

LruCacheEngine::~LruCacheEngine() {}

NonLruCacheEngine::NonLruCacheEngine() {}
NonLruCacheEngine:: ~NonLruCacheEngine(){}

Status NonLruCacheEngine::PutValue(std::string partition_path, int column_id, CacheEntryHolder cache_entry_holder) {
 }

} // namespace pegasus