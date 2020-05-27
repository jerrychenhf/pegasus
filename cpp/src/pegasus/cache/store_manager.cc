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
#include "cache/memory_store.h"
#include "cache/dcpmm_store.h"

namespace pegasus {

const std::string StoreManager::STORE_ID_DRAM = "DRAM";
const std::string StoreManager::STORE_ID_DCPMM = "DCPMM";
const std::string StoreManager::STORE_PROPERTY_PATH = "path";
const std::string StoreManager::STORE_ID_FILE = "FILE";

StoreManager::~StoreManager(){
}

StoreManager::StoreManager() {
}

Status StoreManager::Init() {
  WorkerExecEnv* env =  WorkerExecEnv::GetInstance();
  const StoreInfos& store_infos = env->GetStores();
  
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

Status StoreManager::GetStore(const std::string& id, Store** store) {
  auto  entry = stores_.find(id);
  if (entry == stores_.end()) {
    stringstream ss;
    ss << "Failed to get the store in store manager with id: " << id;
    LOG(ERROR) << ss.str();
    return Status::UnknownError(ss.str());
  }
  *store = entry->second.get();
  return Status::OK();
}

Status StoreManager::GetStore(Store::StoreType store_type, Store** store) {
  if (store_type == Store::StoreType::MEMORY) {
    return GetStore(STORE_ID_DRAM, store);
  } else if (store_type == Store::StoreType::DCPMM) {
    return GetStore(STORE_ID_DCPMM, store);
  } else {
    return Status::Invalid("Invalid store type!");
  }
}

} // namespace pegasus