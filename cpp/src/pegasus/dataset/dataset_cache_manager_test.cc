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
#include <gtest/gtest.h>
#include "test/gtest-util.h"
#include "dataset/dataset_cache_manager.h"
#include "pegasus/runtime/worker_exec_env.h"

namespace pegasus {

TEST(DatasetCacheManagerTest, Unit) {

  WorkerExecEnv exec_env;
  ABORT_IF_ERROR(exec_env.Init());
  std::shared_ptr<DatasetCacheManager> dataset_cache_manager(new DatasetCacheManager());
  dataset_cache_manager->Init();
  
  std::string dataset_path = "hdfs://10.239.47.55:9000/genData2/customer";
  std::string partition_path = "hdfs://10.239.47.55:9000/genData2/customer/part-00000-1fafbf9f-6edf-4f8f-8b51-268708b6f6c5-c000.snappy.parquet";
  std::vector<int> column_indices = {0};

  RequestIdentity request_identity = RequestIdentity(dataset_path, partition_path, column_indices);
  
  auto f0 = arrow::field("c_customer_sk", arrow::int64());
  std::vector<std::shared_ptr<arrow::Field>> fields = {f0};
  auto schema = std::make_shared<arrow::Schema>(fields);
  request_identity.set_schema(schema);
  std::unique_ptr<rpc::FlightDataStream> data_stream;
  dataset_cache_manager->GetDatasetStream(&request_identity, &data_stream);
  ASSERT_EQ(1, data_stream->schema()->num_fields());
}

}
PEGASUS_TEST_MAIN();