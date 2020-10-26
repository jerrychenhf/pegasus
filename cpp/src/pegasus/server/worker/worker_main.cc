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

#include "common/init.h"
#include "common/location.h"
#include "common/status.h"
#include "util/global_flags.h"
#include "util/test-info.h"
#include "runtime/worker_exec_env.h"
#include "server/worker/worker.h"

using namespace std;
using namespace pegasus;

int main(int argc, char** argv) {
  InitCommonRuntime(argc, argv, TestInfo::NON_TEST);

  WorkerExecEnv exec_env;
  ABORT_IF_ERROR(exec_env.Init());
  
  std::unique_ptr<Worker> worker(new Worker(&exec_env));
  
  ABORT_IF_ERROR(worker->Init());
  ABORT_IF_ERROR(worker->Start());
  return 0;
}
