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

#ifndef PEGASUS_WORKER_EXEC_ENV_H
#define PEGASUS_WORKER_EXEC_ENV_H

#include "dataset/cache_engine.h"
#include "runtime/exec_env.h"

using namespace std;

namespace pegasus {
  
class StoreManager;

/// There should only be one ExecEnv instance.
/// It should always be accessed by calling WorkerExecEnv::GetInstance().
class WorkerExecEnv : public ExecEnv {
 public:
  WorkerExecEnv();

  WorkerExecEnv(const std::string& hostname, int32_t port, const std::string& store_types);

  static WorkerExecEnv* GetInstance() { return exec_env_; }

  Status Init();

  std::string GetWorkerGrpcHost();

  int32_t GetWorkerGrpcPort();

  std::unordered_map<string, long> GetStoresInfo();

  std::vector<CacheEngine::CachePolicy> GetCachePolicies();

  std::unordered_map<string, long>  GetCacheStoresInfo();
  
  std::shared_ptr<StoreManager> GetStoreManager() {
    return store_manager_;
  }

 private:
  static WorkerExecEnv* exec_env_;

  std::string worker_grpc_hostname_;
  int32_t worker_grpc_port_;
  std::unordered_map<string, long> stores_info_;
  std::vector<Store::StoreType> store_types_;
  std::unordered_map<string, long> cache_stores_info_; // string: store type
  std::vector<CacheEngine::CachePolicy> cache_policies_;
  
  std::shared_ptr<StoreManager> store_manager_;
};

} // namespace pegasus

#endif  // PEGASUS_WORKER_EXEC_ENV_H