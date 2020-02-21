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

#ifndef PEGASUS_METADATA_H
#define PEGASUS_METADATA_H

#include <vector>
#include <string>

#include "catalog/pegasus_catalog.h"
#include "catalog/spark_catalog.h"
#include "dataset/dataset_request.h"
#include "common/status.h"

namespace pegasus {

class TableMetadata {
 public:
  TableMetadata();

  std::string location_uri;
};

class PartitionMetadata {
 public:
  PartitionMetadata();

  std::string location_uri;
};

class MetadataManager {
 public:
  MetadataManager();

  std::string GetProvider(DataSetRequest* dataset_request);
  Status GetTableMeta(DataSetRequest* dataset_request, std::shared_ptr<TableMetadata>* table_meta);
  Status GetPartitionMeta(std::shared_ptr<std::vector<PartitionMetadata>>* partition_meta);

 private:
  // provider: spark catalog or pegasus catalog
  std::string provider;
  // file format for this table, e.g. parquet, orc, etc.
  std::string file_format;
  TableMetadata table_meta;
  std::vector<PartitionMetadata> partitions_meta;
};

} // namespace pegasus

#endif  // PEGASUS_METADATA_H