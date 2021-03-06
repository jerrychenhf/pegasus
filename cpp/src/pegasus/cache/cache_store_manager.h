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

#ifndef PEGASUS_CACHE_STORE_MANAGER_H
#define PEGASUS_CACHE_STORE_MANAGER_H

#include <vector>
#include <unordered_map>
#include "common/status.h"
#include "cache/cache_store.h"
#include "runtime/worker_config.h"

namespace pegasus {

class CacheStoreManager {
  public:
   CacheStoreManager();
   ~CacheStoreManager();

   Status Init(const CacheStoreInfos& cache_store_infos);
   
   Status GetCacheStore(CacheStore** cache_store);
  private:
   std::unordered_map<std::string, std::shared_ptr<CacheStore>> cached_stores_;

   Status GetCacheStore(const std::string& id, CacheStore** cache_store);   
  public:
   static const std::string CACHE_STORE_ID_DRAM;
   static const std::string CACHE_STORE_ID_DCPMM;
};
} // namespace pegasus                              

#endif  // PEGASUS_CACHE_STORE_MANAGER_H