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

#include <string>

#include "pegasus/util/global_flags.h"

DEFINE_string(hostname, "", "Hostname to use for this daemon. If not set, the system default will be used");
DEFINE_int32(planner_port, 30001, "port on which planner is exported");
DEFINE_int32(worker_port, 30002, "port on which worker is exported");

DEFINE_string(storage_plugin_type, "HDFS", "StoragePluginType");
