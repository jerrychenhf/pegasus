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
#include <memory>
#include <iostream>
#include <gtest/gtest.h>
#include "test/gtest-util.h"
#include "dataset/dataset_service.h"
#include "pegasus/runtime/exec_env.h"
#include "pegasus/dataset/dataset_distributor.h"
#include "consistent_hashing.h"
#include "dataset_request.h"

//#include "arrow/ipc/test_common.h"
//#include "arrow/status.h"
//#include "arrow/testing/gtest_util.h"
//#include "arrow/testing/util.h"
//#include "arrow/util/make_unique.h"

namespace pegasus
{
#if 1
TEST(DatasetServiceTest, ConHashInit)
{
  // 
//  std::cout << "ConHashInit..." << std::endl;
  auto distributor = std::make_shared<ConsistentHashRing>();
  // setup the distribution engine
  distributor->SetupDist();
//  std::cout << "ConHash SetupDist() done." << std::endl;

//  std::cout << "...ConHashInit" << std::endl;
}
#endif
#if 1
TEST(DatasetServiceTest, ConHashBasic)
{
  // 
  auto distributor = std::make_shared<ConsistentHashRing>();

  // generate validloc and update the distributor
//  std::shared_ptr<Location> loc1 = std::make_shared<Location>();
  Location location1, location2, location3;
  Location::ForGrpcTcp("localhost", 10010, &location1);
  Location::ForGrpcTls("localhost", 10010, &location2);
  Location::ForGrpcUnix("/tmp/test.sock", &location3);
  std::shared_ptr<std::vector<Location>> validlocs = std::make_shared<std::vector<Location>>();
  validlocs->push_back(location1);
  validlocs->push_back(location2);
  validlocs->push_back(location3);
  std::shared_ptr<std::vector<int>> nodecacheMBs = std::make_shared<std::vector<int>>();
  nodecacheMBs->push_back(1024);
  nodecacheMBs->push_back(1024);
  nodecacheMBs->push_back(1024);
  distributor->PrepareValidLocations(validlocs, nodecacheMBs);

  // setup the distribution engine
  distributor->SetupDist();

  std::string test_dataset_path = "hostnameplusfolderpath";

  // test get distlocation
  distributor->GetLocation(Identity(test_dataset_path, "partitionfile1"));

  // generate partitions
  auto partitions = std::make_shared<std::vector<Partition>>();
  partitions->push_back(Partition(Identity(test_dataset_path, "partitionfile1")));
  partitions->push_back(Partition(Identity(test_dataset_path, "partitionfile2")));

  // get location for each partition and assign it
  distributor->GetDistLocations(partitions);

  // check the correctness

}
#endif
#if 1
TEST(DatasetServiceTest, DataSetStoreBasic)
{
  Status st;
  std::unique_ptr<PlannerExecEnv> exec_env_(new PlannerExecEnv());
  auto dataset_store_test = std::unique_ptr<DataSetStore>(new DataSetStore);
  std::string test_dataset_path = "hostnameplusfolderpath";
  std::shared_ptr<DataSet> pds = nullptr;

  // create and insert a dataset
  auto catalog_manager = std::make_shared<CatalogManager>();
  auto dsbuilder = std::make_shared<DataSetBuilder>(catalog_manager);
  DataSetRequest dataset_request;
  dataset_request.set_dataset_path(test_dataset_path);
  // Status DataSetBuilder::BuildDataset(DataSetRequest* dataset_request,
  //                                    std::shared_ptr<DataSet>* dataset, int distpolicy)
  st = dsbuilder->BuildDataset(&dataset_request, &pds, CONHASH);
  ASSERT_OK(st);
  //Status DataSetStore::InsertDataSet(std::shared_ptr<DataSet> dataset)
  st = dataset_store_test->InsertDataSet(pds);
  ASSERT_OK(st);

  // get the dataset
  //Status DataSetService::GetDataSet(std::string dataset_path, std::shared_ptr<DataSet>* dataset)
  st = dataset_store_test->GetDataSet(test_dataset_path, &pds);
  ASSERT_OK(st);

  // check the dataset
  ASSERT_TRUE(pds->dataset_path() == test_dataset_path);

}
#endif
#if 1
TEST(DatasetServiceTest, DatasetService)
{
  std::unique_ptr<PlannerExecEnv> exec_env_(new PlannerExecEnv());
  auto dataset_service_ = std::unique_ptr<DataSetService>(new DataSetService());
  dataset_service_->Init();
  std::cout << "addressof dataset_service_: " << std::addressof(dataset_service_) << std::endl;
  //  std::cout << "value dataset_service_: " << std::static_cast<uint64_t>(dataset_service_) << std::endl;
  std::cout << "dataset_service_.get(): " << dataset_service_.get() << std::endl;
//  std::cout << "addressof dataset_service_->dataset_store_: "
//            << std::addressof(dataset_service_->dataset_store_) << std::endl;
//  std::cout << "addressof dataset_service_->dataset_store_->planner_metadata_: "
//            << std::addressof(dataset_service_->dataset_store_->planner_metadata_) << std::endl;
//  auto worker_manager_ = exec_env_->get_worker_manager();

//  std::string test_dataset_path = "hostnameplusfolderpath";
//  std::string test_dataset_path = "hdfs://10.239.47.55:9000/genData2/customer/part-00000-1fafbf9f-6edf-4f8f-8b51-268708b6f6c5-c000.snappy.parquet";
  std::string test_dataset_path = "hdfs://10.239.47.55:9000/genData2/customer";
  auto parttftrs = std::make_shared<std::vector<Filter>>();
  // TODO: parse sql cmd here?
  std::unique_ptr<rpc::FlightInfo>* flight_info=nullptr;
  DataSetRequest dataset_request;
  dataset_request.set_dataset_path(test_dataset_path);
  // Status DataSetService::GetFlightInfo(DataSetRequest* dataset_request,
  //                                   std::unique_ptr<rpc::FlightInfo>* flight_info)
  Status st = dataset_service_->GetFlightInfo(&dataset_request, flight_info);
  ASSERT_OK(st);
}
#endif
} // namespace pegasus
PEGASUS_TEST_MAIN();
