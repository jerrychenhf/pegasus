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

#ifndef PEGASUS_DATASET_CACHE_STORE_MANAGER_H
#define PEGASUS_DATASET_CACHE_STORE_MANAGER_H

#include <unordered_map>

#include "pegasus/common/status.h"
#include "pegasus/cache/store.h"
#include "pegasus/cache/memory_pool.h"
#include "pegasus/runtime/exec_env.h"
#include "pegasus/cache/cache_entry_holder.h"

using namespace std;

namespace pegasus {
class DatasetCacheStoreManager {
 public:
  DatasetCacheStoreManager();
  ~DatasetCacheStoreManager();

  Status Init();
  Status GetCacheEntryHolder(string store_type, long capacity, std::shared_ptr<CacheEntryHolder>* cache_entry_holder);
  Status GetStoreAllocator(Store::StoreType store_type, std::shared_ptr<CacheEntryHolder>* cache_entry_holder);
  Store::StoreType GetStorePolicy();

  private:
  std::unordered_map<std::string, std::shared_ptr<CacheEntryHolder>*> available_stores_;
  std::shared_ptr<StoreManager> store_manager_;
};

} // namespace pegasus

#endif  // PEGASUS_DATASET_CACHE_STORE_MANAGER_H