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
#include "pegasus/runtime/exec_env.h"

using namespace pegasus;

namespace pegasus {

// Initialize all the Store Allocator based on the configuration.
DatasetCacheStoreManager::DatasetCacheStoreManager() {
  ExecEnv* env =  ExecEnv::GetInstance();
  std::shared_ptr<StoreFactory> store_factory = env->get_store_factory();
  std::shared_ptr<Store> store;
  std::vector<Store::StoreType> store_types = env->GetConfiguredStoreTypes();
  for(std::vector<Store::StoreType>::iterator it = store_types.begin(); it != store_types.end(); ++it) {
    store_factory->GetStore(*it, &store);
    configured_stores_[store->GetStoreName()] = store;
  }
}

DatasetCacheStoreManager::~DatasetCacheStoreManager() {}

// Get the available store allocators(DRAM > DCPMM > SSD)
Status DatasetCacheStoreManager::GetStoreAllocator(std::shared_ptr<Store>* store) {

    auto entry = configured_stores_.find("MEMORY");
  
    if (entry == configured_stores_.end()) {
       return Status::KeyError("Could not find the store.", Store::StoreType::MEMORY);
    }
    *store = entry->second;
}
} // namespace pegasus