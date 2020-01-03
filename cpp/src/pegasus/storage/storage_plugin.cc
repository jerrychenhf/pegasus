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
#include <vector>

#include "arrow/filesystem/filesystem.h"
#include "arrow/status.h"
#include "arrow/util/uri.h"

#include "pegasus/runtime/exec_env.h"
#include "pegasus/storage/storage_plugin.h"
#include "pegasus/util/consistent_hashing.h"


using namespace std;

namespace pegasus {

using HdfsDriver = arrow::io::HdfsDriver;
using HdfsPathInfo = arrow::io::HdfsPathInfo;
using ObjectType = arrow::io::ObjectType;

HDFSStoragePlugin::HDFSStoragePlugin() {

}

HDFSStoragePlugin::~HDFSStoragePlugin() {

}

Status HDFSStoragePlugin::Init() {
  ExecEnv* env =  ExecEnv::GetInstance();
  conf_.host = env->GetNameNodeHost();
  conf_.port = env->GetNameNodePort();
  conf_.driver = HdfsDriver::LIBHDFS;
}

Status HDFSStoragePlugin::Connect() {

  arrow::Status st = HadoopFileSystem::Connect(&conf_, &client_);
  if (!st.ok()) {
      return Status(StatusCode(st.code()), st.message());
  }
  return Status::OK();
}

Status HDFSStoragePlugin::ListFiles(std::string dataset_path, std::shared_ptr<std::vector<std::string>>* file_list) {

  std::vector<HdfsPathInfo> children;
  arrow::Status st = client_->ListDirectory(dataset_path, &children);

  if (!st.ok()) {
    return Status(StatusCode(st.code()), st.message());
  }
  for (const auto& child_info : children) {
    arrow::internal::Uri uri;
    uri.Parse(child_info.name);
    std::string child_path = uri.path();

    if(child_info.kind == ObjectType::DIRECTORY) {
      ListFiles(child_path, file_list);
    } else if (child_info.kind == ObjectType::FILE)
    {
      file_list->get()->push_back(child_path);
    }
  }
  return Status::OK();
}

StoragePlugin::StoragePluginType HDFSStoragePlugin::GetPluginType() {

}

} // namespace pegasus