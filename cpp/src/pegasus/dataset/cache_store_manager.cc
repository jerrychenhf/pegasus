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

#include "runtime/exec_env.h"
#include "dataset/cache_store_manager.h"

using namespace std;

namespace pegasus {
  CacheStoreManager::CacheStoreManager(){}
  CacheStoreManager::~CacheStoreManager(){}

  Status CacheStoreManager::Init() {
    ExecEnv* env =  ExecEnv::GetInstance();
    std::shared_ptr<Store> store;
    std::unordered_map<string, long> cache_stores_info = env->GetCacheStoresInfo();
    store_manager_ = env->get_store_manager();

    Store::StoreType store_type_;
    for(std::unordered_map<string, long>::iterator it = cache_stores_info.begin(); it != cache_stores_info.end(); ++it) {
      string store_type = it->first;
      long capacity = it->second;
      std::shared_ptr<Store> store;

      if (store_type == "MEMORY") {
        store_type_ = Store::StoreType::MEMORY;
      } else if (store_type == "DCPMM") {
        store_type_ = Store::StoreType::DCPMM;
      } else {
        return Status::Invalid("Invalid store type!");
      }

      store_manager_->GetStore(store_type_, &store);
      std::shared_ptr<CacheStore> cache_store = std::shared_ptr<CacheStore>(new CacheStore(capacity, store));
      cached_stores_.insert(std::make_pair(store_type, cache_store));
    }
  }

  Status CacheStoreManager::GetCacheStore(std::shared_ptr<CacheStore>* cache_store){
    // MEMORY > DCPMM > FILE
    auto entry = cached_stores_.find("MEMORY");
    if (entry == cached_stores_.end()) {
      return Status::KeyError("Could not find  cache store.");
    }
    *cache_store = entry->second;
    return Status::OK();
  }
} // namespace pegasus