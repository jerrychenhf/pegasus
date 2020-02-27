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
  auto distributor = std::make_shared<ConsistentHashRing>();
  // setup the distribution engine
  Status res = distributor->SetupDist();
  EXPECT_ERROR(Status::Invalid(""), res.code());
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
  auto validlocs = std::make_shared<std::vector<Location>>();
  validlocs->push_back(location1);
  validlocs->push_back(location2);
  validlocs->push_back(location3);
  auto nodecacheMBs = std::make_shared<std::vector<int64_t>>();
  nodecacheMBs->push_back(1024);
  nodecacheMBs->push_back(1024);
  nodecacheMBs->push_back(1024);
  distributor->PrepareValidLocations(validlocs, nodecacheMBs);

  // setup the distribution engine
  ASSERT_OK(distributor->SetupDist());

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
  std::string test_dataset_path = "hdfs://10.239.47.55:9000/genData2/customer";
  std::shared_ptr<DataSet> pds = nullptr;

  // create and insert a dataset
  auto catalog_manager = std::make_shared<CatalogManager>();
  auto dsbuilder = std::make_shared<DataSetBuilder>(catalog_manager);
  DataSetRequest dataset_request;
  dataset_request.set_dataset_path(test_dataset_path);

  DataSetRequest::RequestProperties properties;
  properties[DataSetRequest::TABLE_LOCATION] = test_dataset_path;
  properties[DataSetRequest::PROVIDER] = "SPARK";
  dataset_request.set_properties(properties);

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
//  std::cout << "addressof dataset_service_: " << std::addressof(dataset_service_) << std::endl;
  //  std::cout << "value dataset_service_: " << std::static_cast<uint64_t>(dataset_service_) << std::endl;
//  std::cout << "dataset_service_.get(): " << dataset_service_.get() << std::endl;

  rpc::HeartbeatInfo hbinfo;
/*
  HeartbeatType type;
  std::string hostname;
  std::shared_ptr<Location> address;
  std::shared_ptr<NodeInfo> node_info;
    int64_t cache_capacity;
    int64_t cache_free;
*/
  hbinfo.type = rpc::HeartbeatInfo::HeartbeatType::REGISTRATION;
  hbinfo.hostname = "localhost:10010";
  Location location1;
  Location::ForGrpcTcp("localhost", 10010, &location1);
  hbinfo.address = std::make_shared<Location>(location1);
  hbinfo.node_info = std::make_shared<rpc::NodeInfo>();
  hbinfo.node_info->cache_capacity = 1024*1024*1024; //bytes
  hbinfo.node_info->cache_free = 1024*1024*1024; //bytes

  std::unique_ptr<rpc::HeartbeatResult> hbresult;
//Status WorkerManager::Heartbeat(const rpc::HeartbeatInfo& info, std::unique_ptr<rpc::HeartbeatResult>* result)
  exec_env_->get_worker_manager()->Heartbeat(hbinfo, &hbresult);
  hbinfo.type = rpc::HeartbeatInfo::HeartbeatType::HEARTBEAT;
  exec_env_->get_worker_manager()->Heartbeat(hbinfo, &hbresult);

//  std::string test_dataset_path = "hostnameplusfolderpath";
//  std::string test_dataset_path = "hdfs://10.239.47.55:9000/genData2/customer/part-00000-1fafbf9f-6edf-4f8f-8b51-268708b6f6c5-c000.snappy.parquet";
  std::string test_dataset_path = "hdfs://10.239.47.55:9000/genData2/customer";
  auto parttftrs = std::make_shared<std::vector<Filter>>();

  std::unique_ptr<rpc::FlightInfo> flight_info;

  DataSetRequest dataset_request;
  dataset_request.set_dataset_path(test_dataset_path);
  DataSetRequest::RequestProperties properties;

  properties[DataSetRequest::TABLE_LOCATION] = test_dataset_path;
  properties[DataSetRequest::PROVIDER] = "SPARK";
  // properties[DataSetRequest::COLUMN_NAMES] = "a, b, c";
  dataset_request.set_properties(properties);
  rpc::FlightDescriptor fldtr;
  fldtr.type = rpc::FlightDescriptor::DescriptorType::PATH;
  fldtr.cmd = "testcmd";
  fldtr.path.push_back("a sample path");
//  fldtr.properties;
  // Status DataSetService::GetFlightInfo(DataSetRequest* dataset_request,
  //                                   std::unique_ptr<rpc::FlightInfo>* flight_info)
  //                                   const rpc::FlightDescriptor& fldtr);
  Status st = dataset_service_->GetFlightInfo(&dataset_request, &flight_info, fldtr);
  ASSERT_OK(st);

  // check data correctness
  ASSERT_EQ(1, flight_info->endpoints().size());
  const std::vector<rpc::FlightEndpoint> endpoints = flight_info->endpoints();
  for (rpc::FlightEndpoint endpoint : endpoints) {
    ASSERT_EQ(test_dataset_path, endpoint.ticket.getDatasetpath());
    auto locations = endpoint.locations;
    for (auto location : locations) {
      ASSERT_EQ(10010, location.port());
    }
  }
  // 
}
#endif
} // namespace pegasus
PEGASUS_TEST_MAIN();
