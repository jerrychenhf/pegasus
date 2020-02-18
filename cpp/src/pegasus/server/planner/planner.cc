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
#include "runtime/planner_exec_env.h"

DECLARE_string(hostname);
DECLARE_int32(planner_port);

namespace pegasus {

Planner::Planner(PlannerExecEnv* exec_env)
  : exec_env_(exec_env) {
  
}

Planner::~Planner() {
}

Status Planner::Init() {  
  worker_manager_ = exec_env_->get_worker_manager();
  
  dataset_service_ = std::unique_ptr<DataSetService>(new DataSetService());
  RETURN_IF_ERROR(dataset_service_->Init());
  
  planner_table_api_service_ = 
  std::unique_ptr<PlannerTableAPIService>(new PlannerTableAPIService(dataset_service_));
  RETURN_IF_ERROR(planner_table_api_service_->Init());
  
  return Status::OK();
}

Status Planner::Start() {
  LOG(INFO) << "Planner listening on:" << FLAGS_hostname << ":"
              << FLAGS_planner_port;
  RETURN_IF_ERROR(planner_table_api_service_->Serve());

  return Status::OK();
}

} // namespace pegasus