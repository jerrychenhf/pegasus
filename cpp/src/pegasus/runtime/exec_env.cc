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

#include "pegasus/runtime/exec_env.h"
#include "pegasus/util/global_flags.h"

DECLARE_string(planner_hostname);
DECLARE_int32(planner_port);
DECLARE_string(worker_hostname);
DECLARE_int32(worker_port);
DECLARE_string(storage_plugin_type);
DECLARE_string(store_types);

namespace pegasus {

ExecEnv* ExecEnv::exec_env_ = nullptr;

ExecEnv::ExecEnv()
  : ExecEnv(FLAGS_planner_hostname, FLAGS_planner_port,
        FLAGS_worker_hostname, FLAGS_worker_port,
        FLAGS_storage_plugin_type, FLAGS_store_types) {}

ExecEnv::ExecEnv(const std::string& planner_hostname, int32_t planner_port,
    const std::string& worker_hostname, int32_t worker_port,
    const std::string& storage_plugin_type, const std::string& store_types)
  : storage_plugin_factory_(new StoragePluginFactory()), 
    worker_manager_(new WorkerManager()), store_manager_(new StoreManager()) {
      
  planner_grpc_hostname_ = planner_hostname;
  planner_grpc_port_ = planner_port;
  worker_grpc_hostname_ = worker_hostname;
  worker_grpc_port_ = worker_port;
  if(storage_plugin_type == "HDFS") {
    storage_plugin_type_ = StoragePlugin::HDFS;
  } else if(storage_plugin_type == "S3") {
    storage_plugin_type_ = StoragePlugin::S3;
  }

  std::vector<std::string> types;
  boost::split(types, store_types, boost::is_any_of(", "), boost::token_compress_on);
  for(std::vector<std::string>::iterator it = types.begin(); it != types.end(); ++it) {
    if(*it == "MEMORY") {
      store_types_.push_back(Store::StoreType::MEMORY);
    } else if(*it == "DCPMM") {
      store_types_.push_back(Store::StoreType::DCPMM);
    } else if(*it == "FILE") {
      store_types_.push_back(Store::StoreType::FILE);
    }
  }
  // TODO need get the cache policy and store policy from configuration
  cache_policies_.push_back(CacheEngine::LRU);
  // configured_store_size_.push_back(Store::StoreType::MEMORY);

  exec_env_ = this;
}

std::string ExecEnv::GetPlannerGrpcHost() {
  return planner_grpc_hostname_;
}

int32_t ExecEnv::GetPlannerGrpcPort() {
  return planner_grpc_port_;
}

std::string ExecEnv::GetWorkerGrpcHost() {
  return worker_grpc_hostname_;
}

int32_t ExecEnv::GetWorkerGrpcPort() {
  return worker_grpc_port_;
}

StoragePlugin::StoragePluginType const ExecEnv::GetStoragePluginType() {
  return storage_plugin_type_;
}

std::vector<Store::StoreType> ExecEnv::GetStoreTypes() {
  return store_types_;
}

std::unordered_map<string, long>  ExecEnv::GetConfiguredStoreInfo() {
  return configured_store_size_;
}

std::vector<CacheEngine::CachePolicy> ExecEnv::GetCachePolicies() {
  return cache_policies_;
}

std::string ExecEnv::GetNameNodeHost() {
  return namenode_hostname_;
}

int32_t ExecEnv::GetNameNodePort() {
  return namenode_port_;
}

} // namespace pegasus