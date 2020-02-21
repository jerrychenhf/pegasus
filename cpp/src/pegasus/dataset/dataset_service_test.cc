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

TEST(DatasetServiceTest, ConHashBasic)
{
  // 
  auto distributor = std::make_shared<ConsistentHashRing>();

  // generate validloc and update the distributor
  std::shared_ptr<Location> loc1 = std::make_shared<Location>();
  Location::ForGrpcTcp("localhost", 10086, loc1.get());
  std::shared_ptr<std::vector<std::shared_ptr<Location>>> validloc = \
                          std::make_shared<std::vector<std::shared_ptr<Location>>>();
  validloc->push_back(loc1);
  distributor->PrepareValidLocations(validloc);

  // setup the distribution engine
  distributor->SetupDist();

  // generate partitions
  std::string test_dataset_path = "hostnameplusfolderpath";
  auto partitions = std::make_shared<std::vector<Partition>>();
  partitions->push_back(Partition(Identity(test_dataset_path, "partitionfile1")));
  partitions->push_back(Partition(Identity(test_dataset_path, "partitionfile2")));

  // get location for each partition and assign it
  distributor->GetDistLocations(partitions);

  // check the correctness

}

TEST(DatasetServiceTest, DataSetStoreBasic)
{
  Status st;
  std::unique_ptr<PlannerExecEnv> exec_env_(new PlannerExecEnv());
//  auto worker_manager = exec_env_->get_worker_manager();
  auto dataset_store_test = std::unique_ptr<DataSetStore>(new DataSetStore);
//  std::cout << "addressof dataset_store_test: " << std::addressof(dataset_store_test) << std::endl;
//  std::cout << "dataset_store_test.get(): " << dataset_store_test.get() << std::endl;
  std::string test_dataset_path = "hostnameplusfolderpath";
  std::shared_ptr<DataSet> pds = nullptr;

  // create and insert a dataset
  auto metadata_manager = std::make_shared<MetadataManager>();
//  std::cout << "addressof metadata_manager: " << std::addressof(metadata_manager) << std::endl;
//  std::cout << "metadata_manager.get(): " << metadata_manager.get() << std::endl;
  auto dsbuilder = std::make_shared<DataSetBuilder>(metadata_manager);
//  std::cout << "addressof dsbuilder: " << std::addressof(dsbuilder) << std::endl;
//  std::cout << "dsbuilder.get(): " << dsbuilder.get() << std::endl;
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

}

TEST(DatasetServiceTest, DatasetService)
{
  std::unique_ptr<PlannerExecEnv> exec_env_(new PlannerExecEnv());
  auto worker_manager_ = exec_env_->get_worker_manager();
  auto dataset_service_ = std::unique_ptr<DataSetService>(new DataSetService());
  std::cout << "addressof dataset_service_: " << std::addressof(dataset_service_) << std::endl;
  //  std::cout << "value dataset_service_: " << std::static_cast<uint64_t>(dataset_service_) << std::endl;
  std::cout << "dataset_service_.get(): " << dataset_service_.get() << std::endl;
//  std::cout << "addressof dataset_service_->dataset_store_: "
//            << std::addressof(dataset_service_->dataset_store_) << std::endl;
//  std::cout << "addressof dataset_service_->dataset_store_->planner_metadata_: "
//            << std::addressof(dataset_service_->dataset_store_->planner_metadata_) << std::endl;

  std::string test_dataset_path = "hostnameplusfolderpath";
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
