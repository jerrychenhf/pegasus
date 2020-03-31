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
DEFINE_int32(namenode_port, 50070, "NameNode port. If not set, the system default will be used");
DEFINE_string(hdfs_driver, "libhdfs", "HdfsClient driver to choose between libhdfs and libhdfs3 at runtime");

// log
DEFINE_string(log_filename, "",
    "Prefix of log filename - "
    "full path is <log_dir>/<log_filename>.[INFO|WARN|ERROR|FATAL]");
DEFINE_bool(redirect_stdout_stderr, true,
    "If true, redirects stdout/stderr to INFO/ERROR log.");
DEFINE_int32(max_log_files, 10, "Maximum number of log files to retain per severity "
    "level. The most recent log files are retained. If set to 0, all log files are "
    "retained.");

DEFINE_int32(worker_heartbeat_frequency_ms, 3000, "(Advanced) Frequency (in ms) with"
    " which the worker sends heartbeat to planner.");

DEFINE_int32(planner_max_missed_heartbeats, 5, "Maximum number of consecutive "
    "heartbeat messages a worker can miss before being declared failed by the "
    "planner.");

DEFINE_bool(store_dram_enabled, true,
    "If true, enable DRAM store.");
DEFINE_int32(store_dram_capacity_gb, 10,
    "The DRAM store capacity in GB.");

DEFINE_bool(store_dcpmm_enabled, false,
    "If true, enable DCPMM store.");
DEFINE_int32(store_dcpmm_initial_capacity_gb, 100,
    "The DCPMM store initial capacity in GB.");
DEFINE_int32(store_dcpmm_reserved_capacity_gb, 100,
    "The DCPMM store reserved capacity in GB.");
DEFINE_string(storage_dcpmm_path, "",
    "The DCPMM device path.");

DEFINE_double(cache_available_ratio, 0.8,
    "The available cache ratio. It will begin evict when exceeded.");