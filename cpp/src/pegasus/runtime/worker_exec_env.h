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

#include "cache/cache_engine.h"
#include "runtime/exec_env.h"
#include "dataset/dataset_cache_manager.h"

using namespace std;

namespace pegasus {
  
class StoreManager;

typedef std::unordered_map<string, string> StoreProperties;
    
class StoreInfo {
public:
  StoreInfo(const std::string& id, Store::StoreType type, int64_t capacity,
    const std::shared_ptr<StoreProperties>& properties)
    : id_(id), type_(type), capacity_(capacity), properties_(properties) {
  }
  
  ~StoreInfo() {
  }
  
  const std::string& id() { return id_; }
  Store::StoreType type() { return type_; }
  int64_t capacity() const { return capacity_; }
  std::shared_ptr<StoreProperties> properties() { return properties_; }
private:
  std::string id_;
  Store::StoreType type_;
  int64_t capacity_;  
  std::shared_ptr<StoreProperties> properties_;
};

typedef std::unordered_map<string, std::shared_ptr<StoreInfo>> StoreInfos;

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

  const StoreInfos& GetStoreInfos();

  std::vector<CacheEngine::CachePolicy> GetCachePolicies();

  std::unordered_map<string, long>  GetCacheStoresInfo();
  
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

  StoreInfos store_infos_;
  std::unordered_map<string, long> cache_stores_info_; // string: store type
  std::vector<CacheEngine::CachePolicy> cache_policies_;
  
  std::shared_ptr<StoreManager> store_manager_;
  std::shared_ptr<DatasetCacheManager> dataset_cache_manager_;
  
  Status InitStoreInfo();
  
public:
  static const std::string STORE_ID_DRAM;
  static const std::string STORE_ID_DCPMM;
  static const std::string STORE_PROPERTY_PATH;
  
  static const int64_t KILOBYTE = 1024;
  static const int64_t MEGABYTE = KILOBYTE * 1024;
  static const int64_t GIGABYTE = MEGABYTE * 1024;
};

} // namespace pegasus

#endif  // PEGASUS_WORKER_EXEC_ENV_H