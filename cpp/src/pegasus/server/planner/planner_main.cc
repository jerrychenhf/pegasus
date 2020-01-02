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

#include <iostream> 

#include "pegasus/common/location.h"
#include "pegasus/common/server_options.h"
#include "pegasus/util/global_flags.h"
#include "pegasus/runtime/exec_env.h"
#include "pegasus/server/planner/planner.h"

DECLARE_string(planner_hostname);
DECLARE_int32(planner_port);
DECLARE_string(storage_plugin_type);

using namespace std;
using namespace pegasus;

int main(int argc, char** argv) {

  std::shared_ptr<ServerOptions> options(new ServerOptions(FLAGS_planner_hostname,
      FLAGS_planner_port, FLAGS_storage_plugin_type));
  std::unique_ptr<Planner> planner_server(new Planner(options));
  planner_server->Init();
  std::cout << "Planner listening on:" << FLAGS_planner_hostname << ":" << FLAGS_planner_port << std::endl;
  return 0;
}
