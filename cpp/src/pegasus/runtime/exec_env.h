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

#ifndef PEGASUS_EXEC_ENV_H
#define PEGASUS_EXEC_ENV_H

#include "pegasus/common/worker_manager.h"
#include "pegasus/dataset/dataset_store.h"
#include "pegasus/storage/storage_plugin_factory.h"
#include "pegasus/dataset/cache_engine.h"

using namespace std;

namespace pegasus {

/// There should only be one ExecEnv instance. It should always be accessed by calling ExecEnv::GetInstance().
class ExecEnv {
 public:
  ExecEnv();

  ExecEnv(const std::string& planner_hostname, int32_t planner_port,
      const std::string& worker_hostname, int32_t worker_port,
      const std::string& storage_plugin_type, const std::string& store_types);

  static ExecEnv* GetInstance() { return exec_env_; }

  Status Init();

  std::shared_ptr<WorkerManager> get_worker_manager() {
    return worker_manager_; 
  }

  std::shared_ptr<StoragePluginFactory> get_storage_plugin_factory() {
    return storage_plugin_factory_; 
  }

  std::shared_ptr<StoreManager> get_store_manager() {
    return store_manager_;
  }

  std::string GetPlannerGrpcHost();

  int32_t GetPlannerGrpcPort();

  std::string GetWorkerGrpcHost();

  int32_t GetWorkerGrpcPort();

  StoragePlugin::StoragePluginType const GetStoragePluginType();

  std::unordered_map<string, long> GetStoresInfo();

  std::vector<CacheEngine::CachePolicy> GetCachePolicies();

  std::unordered_map<string, long>  GetCacheStoresInfo();

  std::string GetNameNodeHost();

  int32_t GetNameNodePort();

 private:
  static ExecEnv* exec_env_;
  std::shared_ptr<WorkerManager> worker_manager_;
  std::shared_ptr<StoragePluginFactory> storage_plugin_factory_;

  Location location_;
  std::string planner_grpc_hostname_;
  int32_t planner_grpc_port_;
  std::string worker_grpc_hostname_;
  int32_t worker_grpc_port_;
  StoragePlugin::StoragePluginType storage_plugin_type_;
  std::unordered_map<string, long> stores_info_;
  std::vector<Store::StoreType> store_types_;
  std::unordered_map<string, long> cache_stores_info_; // string: store type
  std::vector<CacheEngine::CachePolicy> cache_policies_;
  std::shared_ptr<StoreManager> store_manager_;

  std::string namenode_hostname_;
  int32_t namenode_port_;
};

} // namespace pegasus

#endif  // PEGASUS_EXEC_ENV_H