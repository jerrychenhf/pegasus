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

#include <memory>
#include <unordered_map>

#include "pegasus/dataset/dataset_cache_store_manager.h"

using namespace pegasus;

namespace pegasus {

// Initialize all the Store Allocator based on the configuration.
DatasetCacheStoreManager::DatasetCacheStoreManager() {

}

DatasetCacheStoreManager::~DatasetCacheStoreManager() {}

Status DatasetCacheStoreManager::Init() {
  ExecEnv* env =  ExecEnv::GetInstance();
  std::shared_ptr<Store> store;
  std::unordered_map<string, long> store_infos = env->GetConfiguredStoreInfo();
  store_manager_ = env->get_store_manager();

  for(std::unordered_map<string, long>::iterator it = store_infos.begin(); it != store_infos.end(); ++it) {
      string store_type = it->first;
      long capacity = it->second;
      std::shared_ptr<CacheEntryHolder>* cache_entry_holder;
      GetCacheEntryHolder(store_type, capacity, cache_entry_holder);
      available_stores_[store_type] = std::move(cache_entry_holder);
  }
}

Status DatasetCacheStoreManager::GetCacheEntryHolder(string store_type, long capacity, std::shared_ptr<CacheEntryHolder>* cache_entry_holder) {
    if (store_type == "MEMORY") {
        std::shared_ptr<Store> store;
        store_manager_->GetStore(Store::StoreType::MEMORY, &store);
        store->Allocate(capacity, cache_entry_holder);
    }
}

Status DatasetCacheStoreManager::GetStoreAllocator(Store::StoreType store_type, std::shared_ptr<CacheEntryHolder>* cache_entry_holder) {
    if (store_type == Store::StoreType::MEMORY) {
        auto entry  = available_stores_.find("MEMORY");
        cache_entry_holder = entry->second;
    } else if (store_type == Store::StoreType::DCPMM) {
        auto entry  = available_stores_.find("DCPMM");
        cache_entry_holder = entry->second;
    }
}

Store::StoreType DatasetCacheStoreManager::GetStorePolicy() {
    // MEMORY > DCPMM > FILE
  return Store::StoreType::MEMORY;
}

} // namespace pegasus