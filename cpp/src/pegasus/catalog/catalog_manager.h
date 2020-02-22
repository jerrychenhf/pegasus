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
#include "common/status.h"

namespace pegasus {

class CatalogManager {
 public:
  CatalogManager();

  Status GetCatalog(DataSetRequest* dataset_request, std::shared_ptr<Catalog>* catalog);

 private:
  // provider: spark catalog or pegasus catalog
  std::string provider_;
  std::shared_ptr<SparkCatalog> spark_catalog_;
  std::shared_ptr<PegasusCatalog> pegasus_catalog_;
};

} // namespace pegasus

#endif  // PEGASUS_METADATA_H