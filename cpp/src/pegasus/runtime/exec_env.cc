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

#include "pegasus/runtime/exec_env.h"

namespace pegasus {

ExecEnv::ExecEnv(std::shared_ptr<ServerOptions> options)
  : options_(options), storage_plugin_factory_(new StoragePluginFactory()), 
    worker_manager_(new WorkerManager()), store_factory_(new StoreFactory) {
}

ExecEnv* ExecEnv::exec_env_ = nullptr;

} // namespace pegasus