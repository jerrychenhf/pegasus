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

#ifndef PEGASUS_DATASET_BUILDER_H
#define PEGASUS_DATASET_BUILDER_H

#include "pegasus/catalog/metadata_manager.h"
#include "pegasus/dataset/dataset.h"
#include "pegasus/parquet/parquet_metadata.h"
#include "pegasus/storage/storage_plugin.h"
#include "pegasus/storage/storage_plugin_factory.h"

namespace pegasus {

class DataSetBuilder {
 public:
  DataSetBuilder(std::shared_ptr<MetadataManager> metadata_manager);

  Status BuildDataset(std::string dataset_path, std::shared_ptr<DataSet>* dataset, int distpolicy);

  Status GetSchma(std::shared_ptr<std::string>* schema);

  Status GetDataSetPath(std::shared_ptr<std::string>* path);

  Status GetPartitions(std::shared_ptr<std::vector<Partition>>* partitions);

  Status GetTotalRecords(int64_t* total_records);
    
  Status GetTotalBytes(int64_t* total_bytes);

 private:
//  std::string dataset_path;
//  std::shared_ptr<std::vector<std::string>> file_list_;
//  std::shared_ptr<std::vector<Location>> vectloc_;
  std::shared_ptr<StoragePlugin> storage_plugin_;
  std::shared_ptr<StoragePluginFactory> storage_plugin_factory_;
  std::shared_ptr<MetadataManager> metadata_manager_;
};

} // namespace pegasus

#endif  // PEGASUS_DATASET_BUILDER_H