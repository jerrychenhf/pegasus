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

#include "pegasus/dataset/dataset_store.h"

namespace pegasus {

Status DataSetStore::GetDataSets(std::shared_ptr<std::vector<std::shared_ptr<DataSet>>>* datasets) {
  
  datasets->get()->reserve(planner_metadata_.size());
  for(auto entry : planner_metadata_) {
    datasets->get()->push_back(entry.second);
  } 
  return Status::OK();
}

Status DataSetStore::GetDataSet(std::string dataset_path, std::shared_ptr<DataSet>* dataset) {
  
  auto entry = planner_metadata_.find(dataset_path);
  if (entry == planner_metadata_.end()) {
    return Status::KeyError("Could not find dataset.", dataset_path);
  }
  auto find_dataset = entry->second;
  *dataset = std::shared_ptr<DataSet>(new DataSet(*find_dataset));
  return Status::OK();
}

Status DataSetStore::InsertDataSet(std::shared_ptr<DataSet> dataset) {
   
  std::string key = dataset->dataset_path();
  planner_metadata_[key] = std::move(dataset);
  return Status::OK();
}

Status DataSetStore::InsertEndPoint(std::string dataset_path, std::shared_ptr<Endpoint> new_endpoint) {
   
  std::shared_ptr<DataSet>* dataset;
  GetDataSet(dataset_path, dataset);
  std::vector<Endpoint> endpoints = (*dataset)->endpoints();

  DataSet::Data data = DataSet::Data();
  data.dataset_path = dataset_path;
  endpoints.push_back(*new_endpoint);
  data.endpoints = endpoints;
  data.total_records = (*dataset)->total_records();
  data.total_bytes = (*dataset)->total_bytes();
  DataSet value(data);

  planner_metadata_[dataset_path] = std::unique_ptr<DataSet>(new DataSet(value));
}

} // namespace pegasus