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

DECLARE_string(hostname);
DECLARE_int32(worker_port);
DECLARE_string(storage_plugin_type);
DECLARE_string(store_types);

namespace pegasus {

WorkerExecEnv* WorkerExecEnv::exec_env_ = nullptr;

WorkerExecEnv::WorkerExecEnv()
  : WorkerExecEnv(FLAGS_hostname, FLAGS_worker_port, FLAGS_store_types) {}

WorkerExecEnv::WorkerExecEnv(const std::string& hostname, int32_t worker_port,
    const std::string& store_types) {
      
  worker_grpc_hostname_ = hostname;
  worker_grpc_port_ = worker_port;

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
  stores_info_.insert(std::make_pair("MEMORY", 1000));
  cache_stores_info_.insert(std::make_pair("MEMORY", 100));

  exec_env_ = this;
}

Status WorkerExecEnv::Init() {
  RETURN_IF_ERROR(ExecEnv::Init());
  return Status::OK();
}

std::string WorkerExecEnv::GetWorkerGrpcHost() {
  return worker_grpc_hostname_;
}

int32_t WorkerExecEnv::GetWorkerGrpcPort() {
  return worker_grpc_port_;
}

std::unordered_map<string, long> WorkerExecEnv::GetStoresInfo() {
  return stores_info_;
}

std::unordered_map<string, long>  WorkerExecEnv::GetCacheStoresInfo() {
  return cache_stores_info_;
}

std::vector<CacheEngine::CachePolicy> WorkerExecEnv::GetCachePolicies() {
  return cache_policies_;
}

} // namespace pegasus