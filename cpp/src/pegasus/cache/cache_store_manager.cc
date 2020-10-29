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

#include "cache/cache_store_manager.h"
#include "runtime/worker_exec_env.h"
#include "cache/store_manager.h"
#include "common/logging.h"

using namespace std;

namespace pegasus {
  
const std::string CacheStoreManager::CACHE_STORE_ID_DRAM = "DRAM";
const std::string CacheStoreManager::CACHE_STORE_ID_DCPMM = "DCPMM";
  
  CacheStoreManager::CacheStoreManager(){
  }
  
  CacheStoreManager::~CacheStoreManager(){
  }

  Status CacheStoreManager::Init(const CacheStoreInfos& cache_store_infos) {
    WorkerExecEnv* env =  WorkerExecEnv::GetInstance();
  
    for(CacheStoreInfos::const_iterator it = cache_store_infos.begin();
      it != cache_store_infos.end(); ++it) {
      std::shared_ptr<CacheStoreInfo> cache_store_info = it->second;
      
      Store* store = NULL;
      RETURN_IF_ERROR(env->GetStoreManager()->GetStore(cache_store_info->store_id(), &store));
      
      std::shared_ptr<CacheStore> cache_store =
        std::shared_ptr<CacheStore>(
        new CacheStore(store, cache_store_info->capacity_quote()));
      cached_stores_.insert(std::make_pair(it->first, cache_store));
    }
    
    return Status::OK();
  }
  
  Status CacheStoreManager::GetCacheStore(const std::string& id, CacheStore** cache_store) {
    auto entry = cached_stores_.find(id);
    if (entry == cached_stores_.end()) {
      stringstream ss;
      ss << "Failed to get the cache store in cache store manager with id: " << id;
      LOG(ERROR) << ss.str();
      return Status::UnknownError(ss.str());
    }
    
    *cache_store = entry->second.get();
    return Status::OK();
  }

  Status CacheStoreManager::GetCacheStore(CacheStore** cache_store){
  
    if(cached_stores_.size() == 1) {
      *cache_store = cached_stores_.begin()->second.get();
      return Status::OK();
    } else {
      // choose max available size store
      int64_t max_size;
      std::string max_key;
      for (auto iter = cached_stores_.begin();
       iter != cached_stores_.end(); iter++) {
         std::shared_ptr<CacheStore> cache_store = iter->second;
         max_size = std::max(max_size, cache_store->GetAvailableSize());
         max_key = iter->first;
      }

      Status status = GetCacheStore(max_key, cache_store);

      if(!status.ok()) {
        stringstream ss;
        ss << "Failed to get the cache store in cache store manager with id: " << max_key;
        LOG(ERROR) << ss.str();
        return Status::UnknownError(ss.str());
      }
      
      return Status::OK();
    }
  }
} // namespace pegasus