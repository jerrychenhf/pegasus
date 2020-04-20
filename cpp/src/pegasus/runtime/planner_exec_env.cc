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

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#include "runtime/planner_exec_env.h"
#include "util/global_flags.h"

DECLARE_string(hostname);
DECLARE_int32(planner_port);
DECLARE_string(storage_type);

namespace pegasus {

PlannerExecEnv* PlannerExecEnv::exec_env_ = nullptr;

PlannerExecEnv::PlannerExecEnv()
  : PlannerExecEnv(FLAGS_hostname, FLAGS_planner_port) {}

PlannerExecEnv::PlannerExecEnv(const std::string& hostname, int32_t planner_port)
  : worker_manager_(new WorkerManager()) {
      
  planner_grpc_hostname_ = hostname;
  planner_grpc_port_ = planner_port;

  exec_env_ = this;
}

Status PlannerExecEnv::Init() {
  RETURN_IF_ERROR(ExecEnv::Init());
  RETURN_IF_ERROR(worker_manager_->Init());
  
  return Status::OK();
}

std::shared_ptr<WorkerManager> PlannerExecEnv::get_worker_manager() {
  return worker_manager_; 
}

std::string PlannerExecEnv::GetPlannerGrpcHost() {
  return planner_grpc_hostname_;
}

int32_t PlannerExecEnv::GetPlannerGrpcPort() {
  return planner_grpc_port_;
}

} // namespace pegasus