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

//TODO use the concurrent_hash_map
//#include "tbb/concurrent_hash_map.h"
//using namespace tbb;

#include <memory>
#include <unordered_map>

#include "pegasus/catalog/pegasus_catalog.h"
#include "pegasus/catalog/spark_catalog.h"
#include "pegasus/dataset/dataset_service.h"
#include "pegasus/dataset/flightinfo_builder.h"
#include "pegasus/util/consistent_hashing.h"

namespace pegasus {

DataSetService::DataSetService() {

}

DataSetService::~DataSetService() {
  
}

Status DataSetService::Init() {
  ExecEnv* env =  ExecEnv::GetInstance();
  dataset_store_ = std::unique_ptr<DataSetStore>(new DataSetStore);
  worker_manager_ = env->GetInstance()->get_worker_manager();
  metadata_manager_ = std::make_shared<MetadataManager>();
  return Status::OK();
}

Status DataSetService::GetDataSets(std::shared_ptr<std::vector<std::shared_ptr<DataSet>>>* datasets) {
  dataset_store_->GetDataSets(datasets);
  return Status::OK();
}

Status DataSetService::GetDataSet(std::string dataset_path, std::shared_ptr<DataSet>* dataset) {

  dataset_store_->GetDataSet(dataset_path, dataset);
  if (dataset == NULL) {
    CacheDataSet(dataset_path, dataset, CONHASH);
  }

  return Status::OK();
}

Status DataSetService::CacheDataSet(std::string dataset_path, std::shared_ptr<DataSet>* dataset, int distpolicy) {

  // build the dataset and insert it to dataset store.
  auto dsbuilder = std::make_shared<DataSetBuilder>(metadata_manager_);
  // Status BuildDataset(std::shared_ptr<DataSet>* dataset);
  dsbuilder->BuildDataset(dataset_path, dataset, distpolicy);
  dataset_store_->InsertDataSet(std::shared_ptr<DataSet>(*dataset));

  return Status::OK();
}

/// Build FlightInfo from DataSet.
Status DataSetService::GetFlightInfo(std::string dataset_path, std::vector<Filter>* parttftrs, std::unique_ptr<rpc::FlightInfo>* flight_info) {

  std::shared_ptr<DataSet> dataset;
  Status st = GetDataSet(dataset_path, &dataset);
  if (!st.ok()) {
    return st;
  }
  std::shared_ptr<ResultDataSet> rdataset;
  // Filter dataset
  st = FilterDataSet(parttftrs, dataset, &rdataset);
  if (!st.ok()) {
    return st;
  }
  flightinfo_builder_ = std::shared_ptr<FlightInfoBuilder>(new FlightInfoBuilder(rdataset));
  return flightinfo_builder_->BuildFlightInfo(flight_info);
}

Status DataSetService::FilterDataSet(std::vector<Filter>* parttftr, std::shared_ptr<DataSet> dataset, std::shared_ptr<ResultDataSet>* resultdataset)
{
  //TODO: filter the dataset 
  return Status::OK();
}

/// Build FlightInfos from DataSets.
Status DataSetService::GetFlightListing(std::unique_ptr<rpc::FlightListing>* listings) {
    
  std::shared_ptr<std::vector<std::shared_ptr<DataSet>>> datasets;
  GetDataSets(&datasets);

  auto rdatasets = std::make_shared<std::vector<std::shared_ptr<ResultDataSet>>>();
  //TODO: fill the resultdataset
/*  for (auto ds:(*datasets))
  {
    ResultDataSet::Data dd;
    dd.dataset_path = ds->dataset_path();
    dd.partitions = ds->partitions();
    dd.total_bytes = ds->total_bytes();
    dd.total_records = ds->total_records();
    rdatasets->push_back(&ResultDataSet(dd));
  }
*/
  flightinfo_builder_ = std::shared_ptr<FlightInfoBuilder>(new FlightInfoBuilder(rdatasets));
  flightinfo_builder_->BuildFlightListing(listings);
  return Status::OK();
}

} // namespace pegasus