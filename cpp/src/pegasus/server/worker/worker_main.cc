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

/*
#include "pegasus/common/location.h"
#include "pegasus/util/global_flags.h"
#include "pegasus/runtime/exec_env.h"
#include "pegasus/server/worker/worker.h"
#include "pegasus/common/server_options.h"

DECLARE_string(hostname);
DECLARE_int32(worker_port);
DECLARE_string(store_type);
DECLARE_string(storage_plugin_type);

int WorkerMain(int argc, char** argv) {
  
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  Location location;
  Location::ForGrpcTcp(FLAGS_hostname, FLAGS_worker_port, &location);
  std::shared_ptr<ServerOptions> options(location, FLAGS_storage_plugin_type, FLAGS_store_type);

  std::unique_ptr<Worker> worker_server(new Worker(options));

  worker_server->Init(options);
  // Exit with a clean error code (0) on SIGTERM
  worker_server->SetShutdownOnSignals({SIGTERM});

  std::cout << "Server listening on host:" << FLAGS_worker_port << std::endl;
  worker_server->Serve();
  return 0;
}
*/
