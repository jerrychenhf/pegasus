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

#include "dataset/dataset_service.h"
#include "util/global_flags.h"
#include "server/planner/planner.h"
#include "storage/storage_plugin.h"

DECLARE_string(planner_hostname);
DECLARE_int32(planner_port);

namespace pegasus {

Planner::Planner() {
  std::unique_ptr<ExecEnv> exec_env_(new ExecEnv());
  worker_manager_ = exec_env_->get_worker_manager();
  dataset_service_ = std::unique_ptr<DataSetService>(new DataSetService());
  planner_table_api_service_ = std::unique_ptr<PlannerTableAPIService>(new PlannerTableAPIService(dataset_service_));
}

Planner::~Planner() {
}

Status Planner::Init() {
  RETURN_IF_ERROR(planner_table_api_service_->Init());
  
  return Status::OK();
}

Status Planner::Start() {
  std::cout << "Planner listening on:" << FLAGS_planner_hostname << ":" << FLAGS_planner_port << std::endl;
  RETURN_IF_ERROR(planner_table_api_service_->Serve());

  return Status::OK();
}

} // namespace pegasus