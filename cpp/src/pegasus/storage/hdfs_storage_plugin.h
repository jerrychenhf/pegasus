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

#ifndef PEGASUS_HDFS_STORAGE_PLUGIN_H
#define PEGASUS_HDFS_STORAGE_PLUGIN_H

#include <arrow/io/hdfs.h>

#include "storage/storage_plugin.h"

namespace pegasus {

using HadoopFileSystem = arrow::io::HadoopFileSystem;
using HdfsConnectionConfig = arrow::io::HdfsConnectionConfig;
using HdfsReadableFile = arrow::io::HdfsReadableFile;
using HdfsPathInfo = arrow::io::HdfsPathInfo;

class HDFSStoragePlugin : public StoragePlugin {
 public:
  HDFSStoragePlugin();
  ~HDFSStoragePlugin();
  Status Init(std::string host, int32_t port) override;
  Status Connect() override;
  Status GetModifedTime(std::string dataset_path, uint64_t* modified_time) override;
  Status ListFiles(std::string dataset_path, std::vector<std::string>* file_list) override;
  StoragePluginType GetPluginType() override;

  Status GetReadableFile(std::string file_path, std::shared_ptr<HdfsReadableFile>* file);
  Status GetPathInfo(std::string dataset_path, HdfsPathInfo* file_info);
  Status ListModifiedTimes(std::string dataset_path, std::vector<int32_t>* modified_time_list) ;
  Status ListSubDirectoryModifiedTimes(std::string dataset_path,
                                       std::vector<int32_t>* modified_time_list);

 private:
  std::shared_ptr<HadoopFileSystem> client_;
  HdfsConnectionConfig conf_;
};

} // namespace pegasus

#endif  // PEGASUS_HDFS_STORAGE_PLUGIN_H