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

DECLARE_string(hostname);
DECLARE_int32(worker_port);
DECLARE_string(storage_plugin_type);

DECLARE_bool(store_dram_enabled);
DECLARE_int32(store_dram_capacity_gb);

DECLARE_bool(store_dcpmm_enabled);
DECLARE_int32(store_dcpmm_capacity_gb);
DECLARE_string(storage_dcpmm_path);

namespace pegasus {
  
const std::string WorkerExecEnv::STORE_ID_DRAM = "MEMORY";
const std::string WorkerExecEnv::STORE_ID_DCPMM = "DCPMM";
const std::string WorkerExecEnv::STORE_PROPERTY_PATH = "path";

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
  
  // TODO need get the cache policy and store policy from configuration
  cache_policies_.push_back(CacheEngine::LRU);
  cache_stores_info_.insert(std::make_pair("MEMORY", 100));

  exec_env_ = this;
}

Status WorkerExecEnv::InitStoreInfo() {
  // if DRAM store configured
  if(FLAGS_store_dram_enabled) {
    // read capacity
    int64_t capacity = ((int64_t) FLAGS_store_dram_capacity_gb) * GIGABYTE;
    
    std::shared_ptr<StoreProperties> properties = std::make_shared<StoreProperties>();
    // read properties
    
    std::shared_ptr<StoreInfo> store_info(
      new StoreInfo(STORE_ID_DRAM, Store::StoreType::MEMORY,
      capacity, properties));
    store_infos_.insert(std::make_pair(STORE_ID_DRAM, store_info));
  }
  
  // if DCPMM store configured
  if(FLAGS_store_dcpmm_enabled) {
    // read capacity
    int64_t capacity = ((int64_t) FLAGS_store_dcpmm_capacity_gb) * GIGABYTE;
    
    std::shared_ptr<StoreProperties> properties = std::make_shared<StoreProperties>();
    // read properties
    properties->insert(std::make_pair(STORE_PROPERTY_PATH, FLAGS_storage_dcpmm_path));
    
    std::shared_ptr<StoreInfo> store_info(
      new StoreInfo(STORE_ID_DCPMM, Store::StoreType::DCPMM,
      capacity, properties));
    store_infos_.insert(std::make_pair(STORE_ID_DCPMM, store_info));
  }
  
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

const StoreInfos& WorkerExecEnv::GetStoreInfos() {
  return store_infos_;
}

std::unordered_map<string, long>  WorkerExecEnv::GetCacheStoresInfo() {
  return cache_stores_info_;
}

std::vector<CacheEngine::CachePolicy> WorkerExecEnv::GetCachePolicies() {
  return cache_policies_;
}

} // namespace pegasus