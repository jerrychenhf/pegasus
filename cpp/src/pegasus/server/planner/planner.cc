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

#include "pegasus/dataset/dataset_service.h"
#include "pegasus/server/planner/planner.h"
#include "pegasus/storage/storage_plugin.h"


namespace pegasus {

Planner::Planner(std::shared_ptr<ServerOptions> options) : options_(options) {
  std::unique_ptr<ExecEnv> exec_env_ = std::unique_ptr<ExecEnv>(new ExecEnv(options_));
  worker_manager_ = exec_env_->get_worker_manager();
  planner_table_api_service_ = std::unique_ptr<PlannerTableAPIService>(new PlannerTableAPIService());
  std::unique_ptr<DataSetService>(new DataSetService());
}

Planner::~Planner() {
}

Status Planner::Init() {
  planner_table_api_service_->Init();
  return Status::OK();
}

} // namespace pegasus