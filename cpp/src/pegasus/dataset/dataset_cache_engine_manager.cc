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
#include "dataset/dataset_cache_engine_manager.h"
#include "runtime/worker_exec_env.h"

using namespace pegasus;

namespace pegasus {

const std::string DatasetCacheEngineManager::ENGINE_ID_LRU = "LRU";
const std::string DatasetCacheEngineManager::ENGINE_ID_NONEVICT = "NONEVICT";

DatasetCacheEngineManager::DatasetCacheEngineManager() {
}

DatasetCacheEngineManager::~DatasetCacheEngineManager() {
}

Status DatasetCacheEngineManager::Init() {
  // Initialize all configurated cache engines.
  WorkerExecEnv* env =  WorkerExecEnv::GetInstance();
  
  const CacheEngineInfos& cache_engine_infos = env->GetCacheEngines();
  std::string cache_policy_type;
  for(CacheEngineInfos::const_iterator it = cache_engine_infos.begin();
    it != cache_engine_infos.end(); ++it) {
    std::shared_ptr<CacheEngineInfo> cache_engine_info = it->second;
    
    std::shared_ptr<CacheEngine> cache_engine;
    if (cache_engine_info->type() == CacheEngine::CachePolicy::LRU) {
      cache_engine = std::shared_ptr<CacheEngine>(
        new LruCacheEngine(cache_engine_info->capacity()));
    } else if (cache_engine_info->type() == CacheEngine::CachePolicy::NonEvict) {
      cache_engine = std::shared_ptr<CacheEngine>(
        new NonEvictionCacheEngine());
    }
    
    RETURN_IF_ERROR(cache_engine->Init(cache_engine_info));
    cached_engines_.insert(std::make_pair(it->first, cache_engine));
  }
  return Status::OK();
}

// Get the specific cache engine based on the available capacity.
Status DatasetCacheEngineManager::GetCacheEngine(
  CacheEngine::CachePolicy cache_policy, std::shared_ptr<CacheEngine>* cache_engine) {
    if (cache_policy == CacheEngine::CachePolicy::LRU) {
      auto entry = cached_engines_.find(ENGINE_ID_LRU);
      if (entry == cached_engines_.end()) {
        stringstream ss;
        ss << "Could not find cache engine with id: " << ENGINE_ID_LRU;
        LOG(ERROR) << ss.str();
        return Status::UnknownError(ss.str());
      }
      *cache_engine = entry->second;
      return Status::OK();
    } else if (cache_policy == CacheEngine::CachePolicy::NonEvict) {
      auto entry = cached_engines_.find(ENGINE_ID_NONEVICT);
      if (entry == cached_engines_.end()) {
        stringstream ss;
        ss << "Could not find cache engine with id: " << ENGINE_ID_NONEVICT;
        LOG(ERROR) << ss.str();
        return Status::UnknownError(ss.str());
      }
      *cache_engine = entry->second;
      return Status::OK();
    } else {
      return Status::Invalid("Invalid cache engine type!");
    }
}
} // namespace pegasus