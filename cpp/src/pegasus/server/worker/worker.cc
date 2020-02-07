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

#include <signal.h>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>

#include "util/global_flags.h"
#include "dataset/dataset_cache_manager.h"
#include "common/logging.h"
#include "server/worker/worker.h"
#include "server/worker/worker_heartbeat.h"

DECLARE_string(hostname);
DECLARE_int32(worker_port);

namespace pegasus {

Worker::Worker() {
  std::unique_ptr<ExecEnv> exec_env_(new ExecEnv());
  worker_table_api_service_ = std::unique_ptr<WorkerTableAPIService>(new WorkerTableAPIService());
  dataset_cache_manager_ = std::unique_ptr<DatasetCacheManager>(new DatasetCacheManager());
  worker_heartbeat_ = std::unique_ptr<WorkerHeartbeat>(new WorkerHeartbeat());
}

Worker::~Worker() {
  
}

Status Worker::Init() {
  RETURN_IF_ERROR(worker_heartbeat_->Init());
  RETURN_IF_ERROR(worker_table_api_service_->Init());
  
  return Status::OK();
}

Status Worker::Start() {
  std::cout << "Worker listening on:" << FLAGS_hostname << ":" << FLAGS_worker_port << std::endl;
  RETURN_IF_ERROR(worker_table_api_service_->Serve());
  
  return Status::OK();
}

} // namespace pegasus