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
#include <chrono>
#include <gtest/gtest.h>
#include "test/gtest-util.h"
#include "dataset/dataset_service.h"
#include "runtime/exec_env.h"
#include "dataset/dataset_distributor.h"
#include "dataset/consistent_hashing.h"
#include "dataset/dataset_request.h"

namespace pegasus
{
#if 1
TEST(DatasetServiceTest, ConHashInit)
{
  auto distributor = std::make_shared<ConsistentHashRing>();
  // setup the distribution engine
  Status res = distributor->SetupDistribution();
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
  auto node_cache_capacities = std::make_shared<std::vector<int64_t>>();
  node_cache_capacities->push_back(1024);
  node_cache_capacities->push_back(1024);
  node_cache_capacities->push_back(1024);
  distributor->PrepareValidLocations(validlocs, node_cache_capacities);

  // setup the distribution engine
  ASSERT_OK(distributor->SetupDistribution());

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
#if 0
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
  properties[DataSetRequest::CATALOG_PROVIDER] = "SPARK";
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
  LOG(INFO) << "========================== DatasetServiceTest, DatasetService ==========================";
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
  hbinfo.node_info->cache_capacity = 1024 * 1024 * 1024; //bytes
  hbinfo.node_info->cache_free = 1024 * 1024 * 1024;     //bytes

  std::unique_ptr<rpc::HeartbeatResult> hbresult;
  //Status WorkerManager::Heartbeat(const rpc::HeartbeatInfo& info, std::unique_ptr<rpc::HeartbeatResult>* result)
  exec_env_->get_worker_manager()->Heartbeat(hbinfo, &hbresult);
  hbinfo.type = rpc::HeartbeatInfo::HeartbeatType::HEARTBEAT;
  exec_env_->get_worker_manager()->Heartbeat(hbinfo, &hbresult);
  LOG(INFO) << "====== node1 registered === : " << hbinfo.hostname;

  //  std::string test_dataset_path = "hostnameplusfolderpath";
  //  std::string test_dataset_path = "hdfs://10.239.47.55:9000/genData2/customer/part-00000-1fafbf9f-6edf-4f8f-8b51-268708b6f6c5-c000.snappy.parquet";
  std::string test_dataset_path = "hdfs://10.239.47.55:9000/genData1000/customer";
  auto parttftrs = std::make_shared<std::vector<Filter>>();

  std::unique_ptr<rpc::FlightInfo> flight_info;

  DataSetRequest dataset_request;
  dataset_request.set_dataset_path(test_dataset_path);
  DataSetRequest::RequestProperties properties;

  properties[DataSetRequest::TABLE_LOCATION] = test_dataset_path;
  properties[DataSetRequest::CATALOG_PROVIDER] = "SPARK";
  // properties[DataSetRequest::COLUMN_NAMES] = "a, b, c";
  dataset_request.set_properties(properties);
  rpc::FlightDescriptor fldtr;
  fldtr.type = rpc::FlightDescriptor::DescriptorType::PATH;
  fldtr.cmd = "testcmd";
  fldtr.path.push_back("a sample path");
  //  fldtr.properties;

  { // first read
    LOG(INFO) << "====== GetFlightInfo === ";
    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    // Status DataSetService::GetFlightInfo(DataSetRequest* dataset_request,
    //                                   std::unique_ptr<rpc::FlightInfo>* flight_info)
    //                                   const rpc::FlightDescriptor& fldtr);
    Status st = dataset_service_->GetFlightInfo(&dataset_request, &flight_info, fldtr);
    ASSERT_OK(st);
    std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
    LOG(INFO) << "time span: " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << " us";

    // check data correctness
    ASSERT_EQ(6, flight_info->endpoints().size());
    const std::vector<rpc::FlightEndpoint> endpoints = flight_info->endpoints();
    for (rpc::FlightEndpoint endpoint : endpoints)
    {
      ASSERT_EQ(test_dataset_path, endpoint.ticket.getDatasetpath());
      auto locations = endpoint.locations;
      for (auto location : locations)
      {
        ASSERT_EQ(10010, location.port());
      }
    }
  } // first read

  { // second read
    LOG(INFO) << "====== GetFlightInfo again === ";
    // get flightinfo again, this time the data should be read from datastore
    flight_info.reset();
    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    Status st = dataset_service_->GetFlightInfo(&dataset_request, &flight_info, fldtr);
    ASSERT_OK(st);
    std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
    LOG(INFO) << "time span: " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << " us";

    // check data correctness
    ASSERT_EQ(6, flight_info->endpoints().size());
    const std::vector<rpc::FlightEndpoint> endpoints = flight_info->endpoints();
    for (rpc::FlightEndpoint endpoint : endpoints)
    {
      ASSERT_EQ(test_dataset_path, endpoint.ticket.getDatasetpath());
      auto locations = endpoint.locations;
      for (auto location : locations)
      {
        ASSERT_EQ(10010, location.port());
      }
    }
  } // second read
}
#endif
#if 1
TEST(DatasetServiceTest, WorkerNodesChange)
{
  LOG(INFO) << "========================== DatasetServiceTest, WorkerNodesChange ==========================";
  std::unique_ptr<PlannerExecEnv> exec_env_(new PlannerExecEnv());
  auto dataset_service_ = std::unique_ptr<DataSetService>(new DataSetService());
  dataset_service_->Init();
  //  std::cout << "addressof dataset_service_: " << std::addressof(dataset_service_) << std::endl;
  //  std::cout << "value dataset_service_: " << std::static_cast<uint64_t>(dataset_service_) << std::endl;
  //  std::cout << "dataset_service_.get(): " << dataset_service_.get() << std::endl;

  rpc::HeartbeatInfo hbinfo;
  std::unique_ptr<rpc::HeartbeatResult> hbresult;
  /*
  HeartbeatType type;
  std::string hostname;
  std::shared_ptr<Location> address;
  std::shared_ptr<NodeInfo> node_info;
    int64_t cache_capacity;
    int64_t cache_free;
*/
  // the first worker node
  hbinfo.type = rpc::HeartbeatInfo::HeartbeatType::REGISTRATION;
  hbinfo.hostname = "127.0.0.1";
  Location location1;
  Location::ForGrpcTcp("127.0.0.1", 10011, &location1);
  hbinfo.address = std::make_shared<Location>(location1);
  hbinfo.node_info = std::make_shared<rpc::NodeInfo>();
  hbinfo.node_info->cache_capacity = 1024 * 1024 * 1024; //bytes
  hbinfo.node_info->cache_free = 1024 * 1024 * 1024;     //bytes

  //Status WorkerManager::Heartbeat(const rpc::HeartbeatInfo& info, std::unique_ptr<rpc::HeartbeatResult>* result)
  exec_env_->get_worker_manager()->Heartbeat(hbinfo, &hbresult);
  hbinfo.type = rpc::HeartbeatInfo::HeartbeatType::HEARTBEAT;
  exec_env_->get_worker_manager()->Heartbeat(hbinfo, &hbresult);
  LOG(INFO) << "====== workernode1 added === : " << hbinfo.hostname;

  // the 2nd worker node
  hbinfo.type = rpc::HeartbeatInfo::HeartbeatType::REGISTRATION;
  hbinfo.hostname = "127.0.0.2";
  Location location2;
  Location::ForGrpcTcp("127.0.0.2", 10012, &location2);
  hbinfo.address = std::make_shared<Location>(location2);
  hbinfo.node_info = std::make_shared<rpc::NodeInfo>();
  hbinfo.node_info->cache_capacity = (int64_t)(2) * (1024 * 1024 * 1024); //bytes
  hbinfo.node_info->cache_free = (int64_t)(2) * (1024 * 1024 * 1024);     //bytes

  //  std::unique_ptr<rpc::HeartbeatResult> hbresult;
  //Status WorkerManager::Heartbeat(const rpc::HeartbeatInfo& info, std::unique_ptr<rpc::HeartbeatResult>* result)
  exec_env_->get_worker_manager()->Heartbeat(hbinfo, &hbresult);
  hbinfo.type = rpc::HeartbeatInfo::HeartbeatType::HEARTBEAT;
  exec_env_->get_worker_manager()->Heartbeat(hbinfo, &hbresult);
  LOG(INFO) << "====== workernode2 added === : " << hbinfo.hostname;

  //  std::string test_dataset_path = "hostnameplusfolderpath";
  //  std::string test_dataset_path = "hdfs://10.239.47.55:9000/genData2/customer/part-00000-1fafbf9f-6edf-4f8f-8b51-268708b6f6c5-c000.snappy.parquet";
  std::string test_dataset_path = "hdfs://10.239.47.55:9000/genData1000/customer";
  auto parttftrs = std::make_shared<std::vector<Filter>>();

  std::unique_ptr<rpc::FlightInfo> flight_info;

  DataSetRequest dataset_request;
  dataset_request.set_dataset_path(test_dataset_path);
  DataSetRequest::RequestProperties properties;

  properties[DataSetRequest::TABLE_LOCATION] = test_dataset_path;
  properties[DataSetRequest::CATALOG_PROVIDER] = "SPARK";
  // properties[DataSetRequest::COLUMN_NAMES] = "a, b, c";
  dataset_request.set_properties(properties);
  rpc::FlightDescriptor fldtr;
  fldtr.type = rpc::FlightDescriptor::DescriptorType::PATH;
  fldtr.cmd = "testcmd";
  fldtr.path.push_back("a sample path");
  //  fldtr.properties;
  { // 1st read
    LOG(INFO) << "====== GetFlightInfo 1st time === ";
    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    // Status DataSetService::GetFlightInfo(DataSetRequest* dataset_request,
    //                                   std::unique_ptr<rpc::FlightInfo>* flight_info)
    //                                   const rpc::FlightDescriptor& fldtr);
    Status st = dataset_service_->GetFlightInfo(&dataset_request, &flight_info, fldtr);
    ASSERT_OK(st);
    std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
    LOG(INFO) << "time span: " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << " us";
  }

  // worker node3 is added
  // the 3rd worker node
  hbinfo.type = rpc::HeartbeatInfo::HeartbeatType::REGISTRATION;
  hbinfo.hostname = "127.0.0.3";
  Location location3;
  Location::ForGrpcTcp("127.0.0.3", 10013, &location3);
  hbinfo.address = std::make_shared<Location>(location3);
  hbinfo.node_info = std::make_shared<rpc::NodeInfo>();
  hbinfo.node_info->cache_capacity = (int64_t)(2) * (1024 * 1024 * 1024); //bytes
  hbinfo.node_info->cache_free = (int64_t)(2) * (1024 * 1024 * 1024);     //bytes

  //  std::unique_ptr<rpc::HeartbeatResult> hbresult;
  //Status WorkerManager::Heartbeat(const rpc::HeartbeatInfo& info, std::unique_ptr<rpc::HeartbeatResult>* result)
  exec_env_->get_worker_manager()->Heartbeat(hbinfo, &hbresult);
  hbinfo.type = rpc::HeartbeatInfo::HeartbeatType::HEARTBEAT;
  exec_env_->get_worker_manager()->Heartbeat(hbinfo, &hbresult);
  LOG(INFO) << "====== worker node3 added === : " << hbinfo.hostname;

  { // read again
    LOG(INFO) << "====== GetFlightInfo after new worker node added === ";
    flight_info.reset();
    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    Status st = dataset_service_->GetFlightInfo(&dataset_request, &flight_info, fldtr);
    ASSERT_OK(st);
    std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
    LOG(INFO) << "time span: " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << " us";

    // simulate the 2nd worker's heartbeat, it should get notification
    hbinfo.type = rpc::HeartbeatInfo::HeartbeatType::HEARTBEAT;
    hbinfo.hostname = "127.0.0.2";
    Location location2;
    Location::ForGrpcTcp("127.0.0.2", 10012, &location2);
    hbinfo.address = std::make_shared<Location>(location2);
    hbinfo.node_info = std::make_shared<rpc::NodeInfo>();
    hbinfo.node_info->cache_capacity = (int64_t)(2) * (1024 * 1024 * 1024); //bytes
    hbinfo.node_info->cache_free = (int64_t)(2) * (1024 * 1024 * 1024);     //bytes

    exec_env_->get_worker_manager()->Heartbeat(hbinfo, &hbresult);
  }

#if 0
  // check data correctness
  ASSERT_EQ(6, flight_info->endpoints().size());
  const std::vector<rpc::FlightEndpoint> endpoints = flight_info->endpoints();
  for (rpc::FlightEndpoint endpoint : endpoints) {
    ASSERT_EQ(test_dataset_path, endpoint.ticket.getDatasetpath());
    auto locations = endpoint.locations;
    for (auto location : locations) {
      ASSERT_EQ(10011, location.port());
    }
  }
#endif
  //
}
#endif
} // namespace pegasus
PEGASUS_TEST_MAIN();
