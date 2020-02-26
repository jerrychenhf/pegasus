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

#include "runtime/exec_env.h"
#include "runtime/worker_config.h"

using namespace std;

namespace pegasus {

class StoreManager;
class DatasetCacheManager;

/// There should only be one ExecEnv instance.
/// It should always be accessed by calling WorkerExecEnv::GetInstance().
class WorkerExecEnv : public ExecEnv {
 public:
 
  WorkerExecEnv();

  WorkerExecEnv(const std::string& hostname, int32_t port);

  static WorkerExecEnv* GetInstance() { return exec_env_; }

  Status Init();

  std::string GetWorkerGrpcHost();

  int32_t GetWorkerGrpcPort();

  const StoreInfos& GetStores();
  const CacheEngineInfos& GetCacheEngines();
  
  std::shared_ptr<StoreManager> GetStoreManager() {
    return store_manager_;
  }

  std::shared_ptr<DatasetCacheManager> GetDatasetCacheManager() {
    return dataset_cache_manager_;
  }

 private:
  static WorkerExecEnv* exec_env_;

  std::string worker_grpc_hostname_;
  int32_t worker_grpc_port_;

  StoreInfos stores_;
  CacheEngineInfos cache_engines_;
  
  std::shared_ptr<StoreManager> store_manager_;
  std::shared_ptr<DatasetCacheManager> dataset_cache_manager_;
  
  Status InitStoreInfo();
  Status InitCacheEngineInfos();
};

} // namespace pegasus

#endif  // PEGASUS_WORKER_EXEC_ENV_H