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

#include "pegasus/catalog/catalog_manager.h"

namespace pegasus {

const std::string CatalogManager::CATALOG_ID_SPARK = "SPARK";
const std::string CatalogManager::CATALOG_ID_PEGASUS = "PEGASUS";

CatalogManager::CatalogManager()
  : spark_catalog_(new SparkCatalog()), pegasus_catalog_(new PegasusCatalog()) {
}

Status CatalogManager::GetCatalog(DataSetRequest* dataset_request,
                                  std::shared_ptr<Catalog>* catalog) {
  
  const auto properties = dataset_request->get_properties();
  std::unordered_map<std::string, std::string>::const_iterator it = 
      properties.find(DataSetRequest::CATALOG_PROVIDER);
  if (it != properties.end()) {
    if (it->second == CATALOG_ID_SPARK) {
      *catalog = spark_catalog_;
    } else if (it->second == CATALOG_ID_PEGASUS) {
      *catalog = pegasus_catalog_;
    } else {
      return Status::Invalid("Invalid catalog type: ", it->second);
    }
  } else {
    // return default catalog
    *catalog = spark_catalog_;
  }
  return Status::OK();
}

} // namespace pegasus
