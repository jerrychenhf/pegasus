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

#include "runtime/worker_exec_env.h"

namespace pegasus {

StoreManager::~StoreManager(){}

StoreManager::StoreManager() {
}

Status StoreManager::Init() {
  WorkerExecEnv* env =  WorkerExecEnv::GetInstance();
  std::shared_ptr<Store> store;
  std::unordered_map<string, long> stores_info = env->GetStoresInfo();
  for(std::unordered_map<string, long>::iterator it = stores_info.begin();
   it != stores_info.end(); ++it) {
    std::shared_ptr<Store> store;
    if (it->first == "MEMORY") {
      store = std::shared_ptr<MemoryStore>(new MemoryStore(it->second));
      stores_.insert(std::make_pair("MEMORY", store));
    } else if (it->first == "DCPMM") {
      store = std::shared_ptr<DCPMMStore>(new DCPMMStore(it->second));
      stores_.insert(std::make_pair("DCPMM", store));
    } 
  }
  
  return Status::OK();
}

Status StoreManager::GetStore(Store::StoreType store_type, Store** store) {
  if (store_type == Store::StoreType::MEMORY) {
    auto  entry = stores_.find("MEMORY");
    if (entry == stores_.end()) {
      stringstream ss;
      ss << "Failed to get the memory store in store manager.";
      LOG(ERROR) << ss.str();
      return Status::UnknownError(ss.str());
    }
    *store = entry->second.get();
    return Status::OK();
  } else if (store_type == Store::StoreType::DCPMM) {
    auto  entry = stores_.find("DCPMM");
    if (entry == stores_.end()) {
      stringstream ss;
      ss << "Failed to get the DCPMM store in store manager.";
      LOG(ERROR) << ss.str();
      return Status::UnknownError(ss.str());
    }
    *store = entry->second.get();
    return Status::OK();
  } else {
    return Status::Invalid("Invalid store type!");
  }
}

} // namespace pegasus