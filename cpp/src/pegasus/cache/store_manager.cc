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

#include "cache/store_manager.h"
#include "runtime/worker_exec_env.h"

namespace pegasus {

const std::string StoreManager::STORE_ID_DRAM = "MEMORY";
const std::string StoreManager::STORE_ID_DCPMM = "DCPMM";
const std::string StoreManager::STORE_PROPERTY_PATH = "path";

StoreManager::~StoreManager(){
}

StoreManager::StoreManager() {
}

Status StoreManager::Init() {
  WorkerExecEnv* env =  WorkerExecEnv::GetInstance();
  std::shared_ptr<Store> store;
  const StoreInfos& store_infos = env->GetStoreInfos();
  
  for(StoreInfos::const_iterator it = store_infos.begin();
   it != store_infos.end(); ++it) {
    std::shared_ptr<Store> store;
    std::shared_ptr<StoreInfo> store_info = it->second;
    
    if (store_info->type() == Store::StoreType::MEMORY ) {
      store = std::shared_ptr<MemoryStore>(new MemoryStore(store_info->capacity()));
    } else if (store_info->type() == Store::StoreType::DCPMM) {
      store = std::shared_ptr<DCPMMStore>(new DCPMMStore(store_info->capacity()));
    } else {
      return Status::Invalid("Invalid store type specified.");
    }
    
    RETURN_IF_ERROR(store->Init(store_info->properties().get()));
    stores_.insert(std::make_pair(it->first, store));
  }
  
  return Status::OK();
}

Status StoreManager::GetStore(Store::StoreType store_type, Store** store) {
  if (store_type == Store::StoreType::MEMORY) {
    auto  entry = stores_.find(STORE_ID_DRAM);
    if (entry == stores_.end()) {
      stringstream ss;
      ss << "Failed to get the memory store in store manager.";
      LOG(ERROR) << ss.str();
      return Status::UnknownError(ss.str());
    }
    *store = entry->second.get();
    return Status::OK();
  } else if (store_type == Store::StoreType::DCPMM) {
    auto  entry = stores_.find(STORE_ID_DCPMM);
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