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

#include "pegasus/common/status.h"
#include "pegasus/dataset/dataset.h"

namespace pegasus {

class StoragePlugin {
 public:
  StoragePlugin();
  ~StoragePlugin();
  Status Auth(std::string passwd);
  virtual std::string GetSchema(std::string dataset_path) = 0;
  virtual Status GetDataSet(std::string dataset_path, std::unique_ptr<DataSet>* dataset) = 0;
    
  enum StoragePluginType {
    HDFS,
    S3,
  };

  StoragePluginType GetPluginType();

 private:
  StoragePluginType storage_plugin_type_;;
};

class HDFSStoragePlugin : public StoragePlugin {
 public:
  HDFSStoragePlugin();
  ~HDFSStoragePlugin();
  std::string GetSchema(std::string dataset_path) override;
  Status GetDataSet(std::string dataset_path, std::unique_ptr<DataSet>* dataset) override;
  StoragePluginType GetPluginType();
};

} // namespace pegasus

#endif  // PEGASUS_STORAGE_PLUGIN_H