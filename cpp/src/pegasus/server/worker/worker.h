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

#ifndef PEGASUS_WORKER_H
#define PEGASUS_WORKER_H

#include "common/status.h"
#include "dataset/dataset_cache_manager.h"
#include "runtime/worker_exec_env.h"
#include "storage/storage_plugin.h"
#include "server/worker/worker_table_api_service.h"

using namespace std;

namespace pegasus {

class WorkerExecEnv;
class WorkerHeartbeat;

class Worker {
 public:
  Worker(WorkerExecEnv* exec_env);
  ~Worker();
  
  Status Init();
  Status Start();

 private:
  WorkerExecEnv* exec_env_;
  std::shared_ptr<WorkerTableAPIService> worker_table_api_service_;
  std::shared_ptr<WorkerHeartbeat> worker_heartbeat_;
};

} // namespace pegasus

#endif  // PEGASUS_WORKER_H
