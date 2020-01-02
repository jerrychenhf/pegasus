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

#include "pegasus/server/worker/worker.h"
#include "pegasus/cache/cache_manager.h"

namespace pegasus {

Worker::Worker(std::shared_ptr<ServerOptions> options) : options_(options) {
  std::unique_ptr<ExecEnv> exec_env_ = std::unique_ptr<ExecEnv>(new ExecEnv(options_));
  worker_table_api_service_ = std::unique_ptr<WorkerTableAPIService>(new WorkerTableAPIService());
  cache_manager_ = std::unique_ptr<CacheManager>(new CacheManager());
}

Status Worker::Init() {
  worker_table_api_service_->Init();
  return Status::OK();
}

} // namespace pegasus