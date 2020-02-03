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

#include "server/worker/worker_heartbeat.h"
#include "util/global_flags.h"
#include "util/logging.h"

DECLARE_string(planner_hostname);
DECLARE_int32(planner_port);

namespace pegasus {

WorkerHeartbeat::WorkerHeartbeat() {
  
}

WorkerHeartbeat::~WorkerHeartbeat() {
  
}

Status WorkerHeartbeat::Init() {
  //TO DO
  
  return Status::OK();
}

Status WorkerHeartbeat::Start() {
  //TO DO INFO LOG
  //std::cout << "Worker listening on:" << FLAGS_worker_hostname << ":" << FLAGS_worker_port << std::endl;
  
  return Status::OK();
}

} // namespace pegasus