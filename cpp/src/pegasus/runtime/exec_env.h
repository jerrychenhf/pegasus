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

#ifndef PEGASUS_EXEC_ENV_H
#define PEGASUS_EXEC_ENV_H

#include "storage/storage_plugin_factory.h"

using namespace std;

namespace pegasus {

/// There should only be one ExecEnv instance. It should always be accessed by calling ExecEnv::GetInstance().
class ExecEnv {
 public:
  ExecEnv();

  ExecEnv(const std::string& storage_plugin_type, const std::string& namenode_hostname, int32_t port);

  static ExecEnv* GetInstance() { return exec_env_; }

  Status Init();

  std::shared_ptr<StoragePluginFactory> get_storage_plugin_factory();

  StoragePlugin::StoragePluginType const GetStoragePluginType();

  std::string GetNameNodeHost();

  int32_t GetNameNodePort();

 private:
  static ExecEnv* exec_env_;
  std::shared_ptr<StoragePluginFactory> storage_plugin_factory_;
  StoragePlugin::StoragePluginType storage_plugin_type_;

  std::string namenode_hostname_;
  int32_t namenode_port_;
};

} // namespace pegasus

#endif  // PEGASUS_EXEC_ENV_H