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

#ifndef PEGASUS_PLANNER_H
#define PEGASUS_PLANNER_H

#include "common/logging.h"
#include "dataset/dataset_service.h"
#include "storage/storage.h"
#include "server/planner/worker_manager.h"
#include "server/planner/planner_table_api_service.h"

using namespace std;

namespace pegasus {
class PlannerTableAPIService;
class PlannerExecEnv;

class Planner {
 public:
  Planner(PlannerExecEnv* exec_env);
  ~Planner();

  Status Init();
  Status Start();

 private:
  PlannerExecEnv* exec_env_;
  
  std::shared_ptr<WorkerManager> worker_manager_;
  std::shared_ptr<PlannerTableAPIService> planner_table_api_service_;
  std::shared_ptr<DataSetService> dataset_service_;
};

}  // namespace pegasus

#endif  // PEGASUS_PLANNER_H