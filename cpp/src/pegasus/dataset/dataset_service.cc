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

#include "catalog/pegasus_catalog.h"
#include "catalog/spark_catalog.h"
#include "dataset/dataset_service.h"
#include "dataset/flightinfo_builder.h"
#include "consistent_hashing.h"

namespace pegasus {

DataSetService::DataSetService() {

}

DataSetService::~DataSetService() {
  
}

Status DataSetService::Init() {
  PlannerExecEnv* env =  PlannerExecEnv::GetInstance();
  dataset_store_ = std::unique_ptr<DataSetStore>(new DataSetStore);
  worker_manager_ = env->GetInstance()->get_worker_manager();
  metadata_manager_ = std::make_shared<MetadataManager>();
  return Status::OK();
}

Status DataSetService::GetDataSets(std::shared_ptr<std::vector<std::shared_ptr<DataSet>>>* datasets) {

  dataset_store_->GetDataSets(datasets);
  return Status::OK();
}

Status DataSetService::GetDataSet(DataSetRequest* dataset_request, std::shared_ptr<DataSet>* dataset) {

  std::shared_ptr<DataSet> pds = NULL;
  std::string dataset_path = dataset_request->get_dataset_path();
  dataset_store_->GetDataSet(dataset_path, &pds);

  if (pds == NULL) {
    // === CacheDataSet(dataset_path, dataset, CONHASH);
    // build the dataset and insert it to dataset store.
    auto dsbuilder = std::make_shared<DataSetBuilder>(metadata_manager_);
    // Status BuildDataset(std::shared_ptr<DataSet>* dataset);
    dsbuilder->BuildDataset(dataset_request, dataset, CONHASH);
    // Begin Write
    (*dataset)->lockwrite();
#if 0
    // read again to avoid duplicated write
    dataset_store_->GetDataSet(dataset_path, &pds);
    if (pds != NULL)
    {
      (*dataset)->unlockwrite();  // drop this prepared dataset
      *dataset = std::make_shared<DataSet>(pds->GetData()); // fill dataset from pds
      pds->unlockread();
      return Status::OK();
    }
#endif
    // do the write
    dataset_store_->InsertDataSet(std::shared_ptr<DataSet>(*dataset));
    (*dataset)->unlockwrite();
    // End Write
  }
  else
  {
    pds->lockread();
//    *dataset = std::shared_ptr<DataSet>(new DataSet(*pds));
    *dataset = std::make_shared<DataSet>(pds->GetData());
    pds->unlockread();
  }

  return Status::OK();
}

Status DataSetService::CacheDataSet(DataSetRequest* dataset_request, std::shared_ptr<DataSet>* dataset, int distpolicy) {

  // build the dataset and insert it to dataset store.
  auto dsbuilder = std::make_shared<DataSetBuilder>(metadata_manager_);
  // Status BuildDataset(std::shared_ptr<DataSet>* dataset);
  dsbuilder->BuildDataset(dataset_request, dataset, distpolicy);
  // Begin Write
  (*dataset)->lockwrite();
  dataset_store_->InsertDataSet(std::shared_ptr<DataSet>(*dataset));
  (*dataset)->unlockwrite();
  // End Write

  return Status::OK();
}

/// Build FlightInfo from DataSet.
Status DataSetService::GetFlightInfo(DataSetRequest* dataset_request,
                                     std::unique_ptr<rpc::FlightInfo>* flight_info) {

  std::shared_ptr<DataSet> dataset = nullptr;
  Status st = GetDataSet(dataset_request, &dataset);
  if (!st.ok()) {
    return st;
  }
  std::shared_ptr<ResultDataSet> rdataset;
  std::vector<Filter>* parttftrs = dataset_request->get_filters();
  // Filter dataset
  dataset->lockread();
  st = FilterDataSet(parttftrs, dataset, &rdataset);
  dataset->unlockread();
  // Note: we can also release the dataset readlock here, the benefit is it avoids dataset mem copy.
  if (!st.ok()) {
    return st;
  }
  flightinfo_builder_ = std::shared_ptr<FlightInfoBuilder>(new FlightInfoBuilder(rdataset));
  return flightinfo_builder_->BuildFlightInfo(flight_info);
}

Status DataSetService::FilterDataSet(std::vector<Filter>* parttftr, std::shared_ptr<DataSet> dataset,
                                     std::shared_ptr<ResultDataSet>* resultdataset)
{
  //TODO: filter the dataset
//  resultdataset = std::make_shared<ResultDataSet>(dataset->GetData());
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