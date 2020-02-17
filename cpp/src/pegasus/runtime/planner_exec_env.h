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

#ifndef PEGASUS_PLANNER_EXEC_ENV_H
#define PEGASUS_PLANNER_EXEC_ENV_H

#include "server/planner/worker_manager.h"
#include "runtime/exec_env.h"

using namespace std;

namespace pegasus {

/// There should only be one ExecEnv instance.
/// It should always be accessed by calling PlannerExecEnv::GetInstance().
class PlannerExecEnv : public ExecEnv {
 public:
  PlannerExecEnv();

  PlannerExecEnv(const std::string& hostname, int32_t port);

  static PlannerExecEnv* GetInstance() { return exec_env_; }

  Status Init();

  std::shared_ptr<WorkerManager> get_worker_manager();

  std::string GetPlannerGrpcHost();

  int32_t GetPlannerGrpcPort();

 private:
  static PlannerExecEnv* exec_env_;

  std::shared_ptr<WorkerManager> worker_manager_;
  std::string planner_grpc_hostname_;
  int32_t planner_grpc_port_;
};

} // namespace pegasus

#endif  // PEGASUS_PLANNER_EXEC_ENV_H