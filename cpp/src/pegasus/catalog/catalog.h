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

#include "common/status.h"
#include "dataset/dataset_request.h"
#include "dataset/identity.h"
#include "dataset/partition.h"

using namespace std;

namespace pegasus {

class Catalog {
 public:
  virtual Status GetSchema(DataSetRequest* dataset_request,
      std::shared_ptr<arrow::Schema>* schema) = 0;

  enum CatalogType {
    UNKOWN,
    SPARK,
    PEGASUS
  };

  enum FileFormat {
    UNKNOWN,
    PARQUET,
    ORC
  };

  virtual FileFormat GetFileFormat(DataSetRequest* dataset_request) = 0;

  virtual CatalogType GetCatalogType() = 0;

 private:
  CatalogType catalog_type_;
};

} // namespace pegasus

#endif  // PEGASUS_CATALOG_H