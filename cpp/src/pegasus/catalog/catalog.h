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

#ifndef PEGASUS_CATALOG_H
#define PEGASUS_CATALOG_H

#include "dataset/dataset_request.h"
#include "pegasus/common/status.h"

using namespace std;

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

class Catalog {
 public:
  virtual Status GetTableMeta(DataSetRequest* dataset_request,
      std::shared_ptr<TableMetadata>* table_meta) = 0;
  virtual Status GetPartitionMeta(DataSetRequest* dataset_request,
      std::shared_ptr<std::vector<PartitionMetadata>>* partition_meta) = 0;

 private:
  // file format for this table, e.g. parquet, orc, etc.
  std::string file_format_;
  TableMetadata table_meta_;
  std::vector<PartitionMetadata> partitions_meta_;
};

} // namespace pegasus

#endif  // PEGASUS_CATALOG_H