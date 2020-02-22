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

#ifndef PEGASUS_SPARK_CATALOG_H
#define PEGASUS_SPARK_CATALOG_H

#include "catalog/catalog.h"
#include "storage/storage_plugin.h"
#include "storage/storage_plugin_factory.h"

#include <string>

using namespace std;

namespace pegasus {

class SparkCatalog : public Catalog {
 public:
  SparkCatalog();
  ~SparkCatalog();

  Status GetPartitions(DataSetRequest* dataset_request,
      std::shared_ptr<std::vector<Partition>>* partitions);
  
  private:
    std::shared_ptr<StoragePluginFactory> storage_plugin_factory_;
};

} // namespace pegasus

#endif  // PEGASUS_SPARK_CATALOG_H