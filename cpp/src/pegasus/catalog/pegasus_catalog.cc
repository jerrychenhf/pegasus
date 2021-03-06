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

#include "catalog/pegasus_catalog.h"

using namespace std;

namespace pegasus {

PegasusCatalog::PegasusCatalog() {
}

PegasusCatalog::~PegasusCatalog() {
}

Status PegasusCatalog::GetTableLocation(DataSetRequest* dataset_request,
  std::string& table_location) {
  return Status::NotImplemented("Pegasus Catalog not yet implemented.");
}

Status PegasusCatalog::GetSchema(DataSetRequest* dataset_request,
  std::shared_ptr<arrow::Schema>* schema) {
  return Status::NotImplemented("Pegasus Catalog not yet implemented.");
}

Catalog::FileFormat PegasusCatalog::GetFileFormat(DataSetRequest* dataset_request) {
  return FileFormat::UNKNOWN;
}

Catalog::CatalogType PegasusCatalog::GetCatalogType() {
   return CatalogType::PEGASUS;
}

} // namespace pegasus
