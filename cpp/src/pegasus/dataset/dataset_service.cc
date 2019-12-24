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

#include "pegasus/dataset/dataset_builder.h"
#include "pegasus/dataset/dataset_service.h"
#include "pegasus/dataset/flightinfo_builder.h"
#include "pegasus/util/consistent_hashing.h"

namespace pegasus {

Status DataSetService::Init() {
  ExecEnv* env =  ExecEnv::GetInstance();
  dataset_store_ = std::unique_ptr<DataSetStore>(new DataSetStore);
  std::shared_ptr<StoragePluginFactory> storage_plugin_factory_ = env->get_storage_plugin_factory();
  storage_plugin_factory_->GetStoragePlugin(env->GetOptions()->storage_plugin_type_, storage_plugin_);
  worker_manager_ = env->GetInstance()->get_worker_manager();
}

Status DataSetService::GetDataSets(std::shared_ptr<std::vector<std::shared_ptr<DataSet>>>* datasets) {
  dataset_store_->GetDataSets(datasets);
  return Status::OK();
}

Status DataSetService::GetDataSet(std::string dataset_path, std::shared_ptr<DataSet>* dataset) {

  dataset_store_->GetDataSet(dataset_path, dataset);
  if (dataset == NULL) {
    std::shared_ptr<std::vector<std::string>>* file_list;
    storage_plugin_->get()->GetFileList(dataset_path, file_list);

    std::shared_ptr<DataSetBuilder> dataset_builder = 
        std::shared_ptr<DataSetBuilder>(new DataSetBuilder(dataset_path, *file_list));
    
    dataset_builder->BuildDataset(dataset);

  // insert the locations to dataset.
    std::shared_ptr<std::vector<std::shared_ptr<Location>>> worker_locations;
    worker_manager_->GetWorkerLocations(worker_locations);
      //TODO: worker with label
      //std::unique_ptr<ConsistentHashRing>(new ConsistentHashRing(worker_locations))->GetLocation(identity);
    dataset_store_->InsertDataSet(std::shared_ptr<DataSet>(dataset->get()));
  }
  return Status::OK();
}

/// Build FlightInfo from DataSet.
Status DataSetService::GetFlightInfo(std::string dataset_path, std::unique_ptr<FlightInfo>* flight_info) {
    
  std::shared_ptr<DataSet>* dataset;
  GetDataSet(dataset_path, dataset);

  flightinfo_builder_ = std::shared_ptr<FlightInfoBuilder>(new FlightInfoBuilder(*dataset));
  flightinfo_builder_->BuildFlightInfo(flight_info);
}

/// Build FlightInfos from DataSets.
Status DataSetService::GetFlightListing(std::unique_ptr<FlightListing>* listings) {
    
  std::shared_ptr<std::vector<std::shared_ptr<DataSet>>>* datasets;
  GetDataSets(datasets);

  flightinfo_builder_ = std::shared_ptr<FlightInfoBuilder>(new FlightInfoBuilder(*datasets));
  flightinfo_builder_->BuildFlightListing(listings);
}

} // namespace pegasus