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

#include "pegasus/runtime/exec_env.h"
// #include "pegasus/cache/store_manager.h"

namespace pegasus {

StoreManager::~StoreManager(){}

StoreManager::StoreManager() {
  ExecEnv* env =  ExecEnv::GetInstance();
  std::shared_ptr<Store> store;
  std::vector<Store::StoreType> store_types = env->GetStoreTypes();
  for(std::vector<Store::StoreType>::iterator it = store_types.begin(); it != store_types.end(); ++it) {
    std::shared_ptr<Store> store;
    if (*it == Store::StoreType::MEMORY) {
      store = std::shared_ptr<MemoryStore>(new MemoryStore());
      stores_.insert(std::make_pair("MEMORY", store));
    } else if (*it == Store::StoreType::DCPMM) {
      store = std::shared_ptr<DCPMMStore>(new DCPMMStore());
      stores_.insert(std::make_pair("DCPMM", store));
    } 
  }
}

Status StoreManager::GetStore(Store::StoreType store_type, std::shared_ptr<Store>* store) {

  if (store_type == Store::StoreType::MEMORY) {
    auto  entry = stores_.find("MEMORY");
    if (entry == stores_.end()) {
      return Status::KeyError("Could not find  DRAM store.");
    }
    *store = entry->second;
    return Status::OK();
  } else if (store_type == Store::StoreType::DCPMM) {
    auto  entry = stores_.find("DCPMM");
    if (entry == stores_.end()) {
      return Status::KeyError("Could not find  DCPMM store.");
    }
    *store = entry->second;
    return Status::OK();
  } else {
    return Status::Invalid("Invalid store type!");
  }
}

} // namespace pegasus