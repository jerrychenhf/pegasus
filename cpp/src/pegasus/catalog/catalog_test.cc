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

#include <stdlib.h>
#include <stdio.h>
#include <iostream>

#include "arrow/util/uri.h"
#include "arrow/type.h"
#include <gtest/gtest.h>

#include "catalog/catalog.h"
#include "catalog/catalog_manager.h"
#include "dataset/dataset_request.h"
#include "test/gtest-util.h"
#include "runtime/planner_exec_env.h"

namespace pegasus {

TEST(SpakrCatalogTest, Unit) {
  std::unique_ptr<PlannerExecEnv> planner_exec_env_(new PlannerExecEnv());

  //TODO: create a test file.
  std::string dataset_path = "hdfs://10.239.47.55:9000/genData2/customer";

  DataSetRequest dataset_request;
  dataset_request.set_dataset_path(dataset_path);
  DataSetRequest::RequestProperties properties;
  properties["provider"] = "SPARK";
  properties["column_names"] = "a, b, c";
  dataset_request.set_properties(properties);

  std::shared_ptr<CatalogManager> catalog_manager = std::make_shared<CatalogManager>();
  std::shared_ptr<Catalog> catalog;
  ASSERT_OK(catalog_manager->GetCatalog(&dataset_request, &catalog));

  std::shared_ptr<arrow::Schema> schema;
  ASSERT_OK(catalog->GetSchema(&dataset_request, &schema));
  ASSERT_EQ(18, schema->num_fields());

  std::vector<std::string> columns = dataset_request.get_column_names();
  std::vector<std::string> columns_expected= {"a", "b", "c"};
  ASSERT_EQ(3, columns.size());
  ASSERT_EQ(columns_expected, columns);
}

}

PEGASUS_TEST_MAIN();

