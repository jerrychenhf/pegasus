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

#ifndef PEGASUS_STORAGE_PLUGIN_H
#define PEGASUS_STORAGE_PLUGIN_H

#include "arrow/io/hdfs.h"
#include "pegasus/common/status.h"
#include "pegasus/dataset/dataset.h"

namespace pegasus {
using HadoopFileSystem = arrow::io::HadoopFileSystem;
using HdfsConnectionConfig = arrow::io::HdfsConnectionConfig;
using HdfsReadableFile = arrow::io::HdfsReadableFile;

class StoragePlugin {
 public:
  Status Init();
  Status Connect();
  Status Auth(std::string passwd);
  virtual Status ListFiles(std::string dataset_path, std::shared_ptr<std::vector<std::string>>* file_list) = 0;
  virtual Status GetReadableFile(std::string file_path, std::shared_ptr<HdfsReadableFile>* file) = 0;
    
  enum StoragePluginType {
    HDFS,
    S3,
  };

  StoragePluginType GetPluginType();

 private:
  StoragePluginType storage_plugin_type_;
};

class HDFSStoragePlugin : public StoragePlugin {
 public:
  HDFSStoragePlugin();
  ~HDFSStoragePlugin();
  Status Init();
  Status Connect();
  Status ListFiles(std::string dataset_path, std::shared_ptr<std::vector<std::string>>* file_list) override;
  StoragePluginType GetPluginType();
  Status GetReadableFile(std::string file_path, std::shared_ptr<HdfsReadableFile>* file);

 private:
  std::shared_ptr<HadoopFileSystem> client_;
  HdfsConnectionConfig conf_;
};

} // namespace pegasus

#endif  // PEGASUS_STORAGE_PLUGIN_H