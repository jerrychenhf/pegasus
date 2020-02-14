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

// This will be defaulted to the host name returned by the OS.
// This name is used in the principal generated for Kerberos authorization.
DEFINE_string(hostname, "", "Hostname to use for this daemon, also used as part of "
              "the Kerberos principal, if enabled. If not set, the system default will be"
              " used");

// This is used by worker to heartbeat with planner
DEFINE_string(planner_hostname, "", "Hostname of the planner to register with");              

DEFINE_int32(planner_port, 30001, "port on which planner is exported");
DEFINE_int32(worker_port, 30002, "port on which worker is exported");

DEFINE_string(storage_plugin_type, "HDFS", "StoragePluginType");
DEFINE_string(store_types, "MEMORY", "StoreType");

DEFINE_string(namenode_hostname, "localhost", "NameNode hostname. If not set, the system default will be used");
DEFINE_string(namenode_port, "50070", "NameNode port. If not set, the system default will be used");

// log
DEFINE_string(log_filename, "",
    "Prefix of log filename - "
    "full path is <log_dir>/<log_filename>.[INFO|WARN|ERROR|FATAL]");
DEFINE_bool(redirect_stdout_stderr, true,
    "If true, redirects stdout/stderr to INFO/ERROR log.");
DEFINE_int32(max_log_files, 10, "Maximum number of log files to retain per severity "
    "level. The most recent log files are retained. If set to 0, all log files are "
    "retained.");