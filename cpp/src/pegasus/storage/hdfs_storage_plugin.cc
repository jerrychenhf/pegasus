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

#include "storage/hdfs_storage_plugin.h"
#include "dataset/consistent_hashing.h"

#include <boost/algorithm/string.hpp>

DECLARE_string(namenode_hostname);
DECLARE_int32(namenode_port);
DECLARE_string(hdfs_driver);

using namespace std;

namespace pegasus {

using HdfsPathInfo = arrow::io::HdfsPathInfo;
using ObjectType = arrow::io::ObjectType;
using FileStats = arrow::fs::FileStats;


HDFSStoragePlugin::HDFSStoragePlugin() {

}

HDFSStoragePlugin::~HDFSStoragePlugin() {

}

Status HDFSStoragePlugin::Init(std::string host, int32_t port) {
  if (host.empty()) {
    conf_.host = FLAGS_namenode_hostname;
  } else {
    conf_.host = host;
  }
  if (port == -1) {
    conf_.port = FLAGS_namenode_port;;
  } else {
    conf_.port = port;
  }

  arrow::Status arrowStatus = arrow::io::HaveLibHdfs();
  Status status = Status::fromArrowStatus(arrowStatus);
  RETURN_IF_ERROR(status);
  
  return Status::OK();
}

Status HDFSStoragePlugin::Connect() {

  arrow::Status arrowStatus = HadoopFileSystem::Connect(&conf_, &client_);
  Status status = Status::fromArrowStatus(arrowStatus);
  RETURN_IF_ERROR(status);

  return Status::OK();
}

Status HDFSStoragePlugin::GetModifedTime(std::string dataset_path, uint64_t* modified_time) {

  std::vector<int32_t> modified_time_list;
  RETURN_IF_ERROR(ListModifiedTimes(dataset_path, &modified_time_list));
  *modified_time = *std::max_element(modified_time_list.begin(),
      modified_time_list.end());
  
  return Status::OK();
}

Status HDFSStoragePlugin::GetPathInfo(std::string dataset_path, HdfsPathInfo* file_info) {

  arrow::Status arrowStatus = client_->GetPathInfo(dataset_path, file_info);
  RETURN_IF_ERROR(Status::fromArrowStatus(arrowStatus));;
  return Status::OK();
}

Status HDFSStoragePlugin::ListModifiedTimes(std::string dataset_path,
    std::vector<int32_t>* modified_time_list) {

  arrow::io::HdfsPathInfo path_info;
  RETURN_IF_ERROR(GetPathInfo(dataset_path, &path_info));
  modified_time_list->push_back(path_info.last_modified_time);
  RETURN_IF_ERROR(ListSubDirectoryModifiedTimes(dataset_path, modified_time_list));

  return Status::OK();
}

Status HDFSStoragePlugin::ListSubDirectoryModifiedTimes(std::string dataset_path,
    std::vector<int32_t>* modified_time_list) {

  std::vector<HdfsPathInfo> children;
  arrow::Status arrowStatus = client_->ListDirectory(dataset_path, &children);
  RETURN_IF_ERROR(Status::fromArrowStatus(arrowStatus));;
  for (const auto& child_info : children) {
    arrow::internal::Uri uri;
    uri.Parse(child_info.name);
    std::string child_path = uri.path();

    if(child_info.kind == ObjectType::DIRECTORY) {
      modified_time_list->push_back(child_info.last_modified_time);
      ListSubDirectoryModifiedTimes(child_path, modified_time_list);
    }
  }

  return Status::OK();
}

Status HDFSStoragePlugin::ListFiles(std::string dataset_path,
                                    std::vector<std::string>* file_list,
                                    int64_t* total_bytes) {
  
  std::vector<HdfsPathInfo> children;
  arrow::Status arrowStatus = client_->ListDirectory(dataset_path, &children);
  RETURN_IF_ERROR(Status::fromArrowStatus(arrowStatus));;

  for (const auto& child_info : children) {

    arrow::internal::Uri uri;
    uri.Parse(child_info.name);
    std::string child_path = uri.path();

    if(child_info.kind == ObjectType::DIRECTORY) {
      ListFiles(child_path, file_list, total_bytes);
    } else if (child_info.kind == ObjectType::FILE &&
        !boost::algorithm::ends_with(child_path, "_SUCCESS")) {
      file_list->push_back(child_info.name);
      if (nullptr != total_bytes) {
        *total_bytes = *total_bytes + child_info.size;
      }
    }
  }
  return Status::OK();
}

Status HDFSStoragePlugin::GetReadableFile(std::string file_path,
                                          std::shared_ptr<HdfsReadableFile>* file) {

  arrow::Status arrowStatus = client_->OpenReadable(file_path, file);
  return Status::fromArrowStatus(arrowStatus);
}

StoragePlugin::StoragePluginType HDFSStoragePlugin::GetPluginType() {
   return StoragePlugin::HDFS;
}

} // namespace pegasus