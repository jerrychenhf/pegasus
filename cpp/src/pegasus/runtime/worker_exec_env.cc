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

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#include "runtime/worker_exec_env.h"
#include "util/global_flags.h"
#include "cache/store_manager.h"
#include "dataset/dataset_cache_manager.h"

DECLARE_string(hostname);
DECLARE_int32(worker_port);
DECLARE_string(storage_plugin_type);

DECLARE_bool(store_dram_enabled);
DECLARE_int32(store_dram_capacity_gb);

DECLARE_bool(store_dcpmm_enabled);
DECLARE_int32(store_dcpmm_initial_capacity_gb);
DECLARE_int32(store_dcpmm_reserved_capacity_gb);
DECLARE_string(storage_dcpmm_path);

namespace pegasus {

WorkerExecEnv* WorkerExecEnv::exec_env_ = nullptr;

WorkerExecEnv::WorkerExecEnv()
  : WorkerExecEnv(FLAGS_hostname, FLAGS_worker_port) {}

WorkerExecEnv::WorkerExecEnv(const std::string& hostname, int32_t worker_port)
  : worker_grpc_hostname_(hostname),
    worker_grpc_port_(worker_port),
    store_manager_(new StoreManager()),
    dataset_cache_manager_(new DatasetCacheManager()) {

  //TODO improve the store configuration
  InitStoreInfo();
  InitCacheEngineInfos();

  exec_env_ = this;
}

Status WorkerExecEnv::InitStoreInfo() {
  // if DRAM store configured
  if(FLAGS_store_dram_enabled) {
    // read capacity
    int64_t capacity = ((int64_t) FLAGS_store_dram_capacity_gb) * StoreManager::GIGABYTE;
    
    std::shared_ptr<StoreProperties> properties = std::make_shared<StoreProperties>();
    // read properties
    
    std::shared_ptr<StoreInfo> store_info(
      new StoreInfo(StoreManager::STORE_ID_DRAM, Store::StoreType::MEMORY,
      capacity, properties));
    stores_.insert(std::make_pair(StoreManager::STORE_ID_DRAM, store_info));
  }
  
  // if DCPMM store configured
  if(FLAGS_store_dcpmm_enabled) {
    // read capacity
    int64_t initial_capacity = ((int64_t) FLAGS_store_dcpmm_initial_capacity_gb) * StoreManager::GIGABYTE;
    int64_t reserved_capacity = ((int64_t) FLAGS_store_dcpmm_reserved_capacity_gb) * StoreManager::GIGABYTE;
    int available_capacity = initial_capacity - reserved_capacity;
    
    LOG(INFO) << "The initial capacity is " << initial_capacity << " reserved capacity is " 
    << reserved_capacity << " and the available capacity is " << available_capacity;

    if (available_capacity <= 0) {
      return Status::Invalid("The available capacity must be > 0 when dcpmm enabled");
    }
    
    std::shared_ptr<StoreProperties> properties = std::make_shared<StoreProperties>();
    // read properties
    properties->insert(std::make_pair(StoreManager::STORE_PROPERTY_PATH, FLAGS_storage_dcpmm_path));
    
    std::shared_ptr<StoreInfo> store_info(
      new StoreInfo(StoreManager::STORE_ID_DCPMM, Store::StoreType::DCPMM,
      available_capacity, properties));
    stores_.insert(std::make_pair(StoreManager::STORE_ID_DCPMM, store_info));
  }
  
  return Status::OK();
}

Status WorkerExecEnv::InitCacheEngineInfos() {
  // read properties
  std::shared_ptr<CacheEngineProperties> properties = std::make_shared<CacheEngineProperties>();
  
  int64_t capacity = 0;
  for(StoreInfos::const_iterator it = stores_.begin();
    it != stores_.end(); ++it) {
    std::shared_ptr<StoreInfo> store_info = it->second;
    capacity += store_info->capacity();
  }
  
  std::shared_ptr<CacheEngineInfo> cache_engine_info(
      new CacheEngineInfo(DatasetCacheEngineManager::ENGINE_ID_LRU,
      CacheEngine::CachePolicy::LRU, capacity, properties));

  for(StoreInfos::const_iterator it = stores_.begin();
    it != stores_.end(); ++it) {
    std::shared_ptr<StoreInfo> store_info = it->second;
    std::shared_ptr<CacheStoreProperties> properties = std::make_shared<CacheStoreProperties>();
    
    std::shared_ptr<CacheStoreInfo> cache_store_info(
      new CacheStoreInfo(store_info->id(),
      store_info->id(), store_info->capacity(), properties));
    cache_engine_info->add_cache_store(cache_store_info);
  }
  
  cache_engines_.insert(
    std::make_pair(DatasetCacheEngineManager::ENGINE_ID_LRU, cache_engine_info));
  return Status::OK();
}

Status WorkerExecEnv::Init() {
  RETURN_IF_ERROR(ExecEnv::Init());
  RETURN_IF_ERROR(store_manager_->Init());
  RETURN_IF_ERROR(dataset_cache_manager_->Init());
  
  return Status::OK();
}

std::string WorkerExecEnv::GetWorkerGrpcHost() {
  return worker_grpc_hostname_;
}

int32_t WorkerExecEnv::GetWorkerGrpcPort() {
  return worker_grpc_port_;
}

const StoreInfos& WorkerExecEnv::GetStores() {
  return stores_;
}

const CacheEngineInfos& WorkerExecEnv::GetCacheEngines() {
  return cache_engines_;
}

} // namespace pegasus