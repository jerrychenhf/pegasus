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
  std::shared_ptr<StoragePluginFactory> storage_plugin_factory_ = env->get_storage_plugin_factory();
  storage_plugin_factory_->GetStoragePlugin(env->GetStoragePluginType(), &storage_plugin_);
  worker_manager_ = env->GetInstance()->get_worker_manager();
}

Status DataSetService::GetDataSets(std::shared_ptr<std::vector<std::shared_ptr<DataSet>>>* datasets) {
  dataset_store_->GetDataSets(datasets);
  return Status::OK();
}

Status DataSetService::GetDataSet(std::string table_name, std::shared_ptr<DataSet>* dataset) {

  std::unique_ptr<SparkCatalog> spark_catalog = std::unique_ptr<SparkCatalog>(new SparkCatalog());
  std::unique_ptr<TableMetadata> table_meta;
  spark_catalog->GetTableMeta(table_name, &table_meta);
  std::string dataset_path = table_meta->location;
  dataset_store_->GetDataSet(dataset_path, dataset);
  if (dataset == NULL) {
    std::shared_ptr<std::vector<std::string>> file_list;
    storage_plugin_->ListFiles(dataset_path, &file_list);

  // insert the locations to dataset.
    std::shared_ptr<std::vector<std::shared_ptr<Location>>> worker_locations;
    worker_manager_->GetWorkerLocations(worker_locations);

    // setup the identity vector
    std::vector<Identity> vectident;
    for (auto filepath : *file_list)
    {
//    Identity(std::string file_path, int64_t row_group_id, int64_t num_rows, int64_t bytes);
      vectident.push_back(Identity(filepath, 0, 0, 0));
    }

    // get locations vector from identity vector
    ConsistentHashRing* cnhs = new ConsistentHashRing(worker_locations);  //TODO: use smart ptr
    auto vectloc = std::make_shared<std::vector<Location>>();
    *vectloc = cnhs->GetLocations(vectident);
    delete cnhs;

    // build the dataset
//    DataSetBuilder* dsbuilder = new DataSetBuilder(dataset_path, *file_list, vectloc);
    auto dsbuilder = std::make_shared<DataSetBuilder>(dataset_path, file_list, vectloc);
    // Status BuildDataset(std::shared_ptr<DataSet>* dataset);
    std::shared_ptr<DataSet> dataset;
    dsbuilder->BuildDataset(&dataset);
    dataset_store_->InsertDataSet(std::shared_ptr<DataSet>(dataset));
  }
  return Status::OK();
}

/// Build FlightInfo from DataSet.
Status DataSetService::GetFlightInfo(std::string table_name, std::unique_ptr<FlightInfo>* flight_info) {

  std::shared_ptr<DataSet> dataset;
  Status st = GetDataSet(table_name, &dataset);
  if (!st.ok()) {
    return st;
  }
  flightinfo_builder_ = std::shared_ptr<FlightInfoBuilder>(new FlightInfoBuilder(dataset));
  return flightinfo_builder_->BuildFlightInfo(flight_info);
}

/// Build FlightInfos from DataSets.
Status DataSetService::GetFlightListing(std::unique_ptr<FlightListing>* listings) {
    
  std::shared_ptr<std::vector<std::shared_ptr<DataSet>>> datasets;
  GetDataSets(&datasets);

  flightinfo_builder_ = std::shared_ptr<FlightInfoBuilder>(new FlightInfoBuilder(datasets));
  flightinfo_builder_->BuildFlightListing(listings);
  return Status::OK();
}

} // namespace pegasus