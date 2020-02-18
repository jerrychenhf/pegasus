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

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#include "runtime/exec_env.h"
#include "util/global_flags.h"

DECLARE_string(storage_plugin_type);

namespace pegasus {

ExecEnv* ExecEnv::exec_env_ = nullptr;

ExecEnv::ExecEnv()
  : storage_plugin_factory_(new StoragePluginFactory()) {

  exec_env_ = this;
}

std::shared_ptr<StoragePluginFactory> ExecEnv::get_storage_plugin_factory() {
  return storage_plugin_factory_; 
}

} // namespace pegasus