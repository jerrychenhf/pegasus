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

#include "common/status.h"
#include "cache/store.h"
#include "cache/store_manager.h"
#include "dataset/cache_store.h"

namespace pegasus {

class CacheStoreManager {
  public:
   CacheStoreManager();
   ~CacheStoreManager();

   Status Init();
   Status GetCacheStore(std::shared_ptr<CacheStore>* cache_store);

  private:
   std::unordered_map<std::string, std::shared_ptr<CacheStore>> cached_stores_;
   std::shared_ptr<StoreManager> store_manager_;
};
} // namespace pegasus                              

#endif  // PEGASUS_CACHE_STORE_H