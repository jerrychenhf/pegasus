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

#ifndef PEGASUS_DATASET_CACHE_ENGINE_MANAGER_H
#define PEGASUS_DATASET_CACHE_ENGINE_MANAGER_H

#include "common/status.h"
#include "cache/cache_engine.h"


using namespace std;

namespace pegasus {
class DatasetCacheEngineManager {
 public:
  DatasetCacheEngineManager();
  ~DatasetCacheEngineManager();

  Status Init();
  Status GetCacheEngine(CacheEngine::CachePolicy cache_policy, std::shared_ptr<CacheEngine>* cache_engine);

  private:
   std::unordered_map<std::string, std::shared_ptr<CacheEngine>> cached_engines_;
};

} // namespace pegasus

#endif  // PEGASUS_DATASET_CACHE_ENGINE_MANAGER_H