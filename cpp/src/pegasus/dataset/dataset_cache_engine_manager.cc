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
#include "pegasus/dataset/dataset_cache_engine_manager.h"
#include "pegasus/runtime/exec_env.h"

using namespace pegasus;

namespace pegasus {

DatasetCacheEngineManager::DatasetCacheEngineManager() {
    // Initialize all configurated cache engines.
  ExecEnv* env =  ExecEnv::GetInstance();
  
  std::shared_ptr<CacheEngine> cache_engine;
  std::vector<CacheEngine::CachePolicy> cache_policies = env->GetCachePolicies();
  for(std::vector<CacheEngine::CachePolicy>::iterator it = cache_policies.begin(); it != cache_policies.end(); ++it) {
    GetCacheEngine(*it, &cache_engine);
    cache_engines_->push_back(cache_engine);
  }
}

DatasetCacheEngineManager::~DatasetCacheEngineManager() {}

// Get all configurated cache engines.
Status DatasetCacheEngineManager:: ListCacheEngines(std::shared_ptr<std::vector<std::shared_ptr<CacheEngine>>>* cache_engines) {
    cache_engines = &cache_engines_;
}

// Get the specific cache engine based on the available capacity.
Status DatasetCacheEngineManager::GetCacheEngine(CacheEngine::CachePolicy cache_policy, std::shared_ptr<CacheEngine>* cache_engine) {
    if (cache_policy == CacheEngine::CachePolicy::LRU) {
        *cache_engine = std::shared_ptr<CacheEngine>(new LruCacheEngine(100)); // we need also get the cache capacity
         return Status::OK();
    } else if (cache_policy == CacheEngine::CachePolicy::NonLRU) {
        *cache_engine = std::shared_ptr<CacheEngine>(new NonLruCacheEngine);
         return Status::OK();
    } else {
        return Status::Invalid("Invalid cache engine type!");
    }
}
} // namespace pegasus