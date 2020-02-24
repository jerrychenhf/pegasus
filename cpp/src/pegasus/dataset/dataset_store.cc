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

DataSetStore::DataSetStore() {
}

DataSetStore::~DataSetStore() {

}

Status DataSetStore::GetDataSets(std::shared_ptr<std::vector<std::shared_ptr<DataSet>>>* datasets) {
  
//  boost::shared_lock<boost::shared_mutex> rdlock(dssmtx);
//  boost::lock_guard<boost::detail::spinlock> l(dssspl); //TODO: temporarily disable it for unit test
//  boost::lock_guard<SpinLock> l(dssspl);
  datasets->get()->reserve(planner_metadata_.size());
  for(auto entry : planner_metadata_) {
    //TODO: lockread for each dataset
    datasets->get()->push_back(entry.second);
  }
  return Status::OK();
}

Status DataSetStore::GetDataSet(std::string dataset_path, std::shared_ptr<DataSet>* dataset) {

//  boost::shared_lock<boost::shared_mutex> rdlock(dssmtx);
//  boost::lock_guard<boost::detail::spinlock> l(dssspl); //TODO: temp disable
//  boost::lock_guard<SpinLock> l(dssspl);
  auto entry = planner_metadata_.find(dataset_path);
  if (entry == planner_metadata_.end()) {
    return Status::KeyError("Could not find dataset.", dataset_path);
  }
  else
  {
    *dataset = entry->second;
//  auto find_dataset = entry->second;
//  *dataset = std::shared_ptr<DataSet>(new DataSet(*find_dataset));
  }

  return Status::OK();
}

Status DataSetStore::InsertDataSet(std::shared_ptr<DataSet> dataset) {

  std::string key = dataset->dataset_path();
//  boost::unique_lock<boost::shared_mutex> wrlock(dssmtx);
//  boost::lock_guard<boost::detail::spinlock> l(dssspl); //TODO: temp disable
//  boost::lock_guard<SpinLock> l(dssspl);
  planner_metadata_[key] = std::move(dataset);  // why use move here while dataset is a shared_ptr?
  return Status::OK();
}

Status DataSetStore::RemoveDataSet(std::shared_ptr<DataSet> dataset) {

  // TODO: lock the whole dataset_store first?
  std::string key = dataset->dataset_path();
//  boost::unique_lock<boost::shared_mutex> wrlock(dssmtx);
  boost::lock_guard<boost::detail::spinlock> l(dssspl);
//  boost::lock_guard<SpinLock> l(dssspl);
  planner_metadata_.erase(key);
  return Status::OK();
}

//TODO: this interface is no longer needed.
#if 0
//if it is needed in future, its logic should be updated for concurrence
Status DataSetStore::InsertEndPoint(std::string dataset_path, std::shared_ptr<Partition> new_partition) {
  std::shared_ptr<DataSet>* dataset;
  GetDataSet(dataset_path, dataset);
  std::vector<Partition> partitions = (*dataset)->partitions();

  DataSet::Data data = DataSet::Data();
  data.dataset_path = dataset_path;
  partitions.push_back(*new_partition);
  data.partitions = partitions;
  data.total_records = (*dataset)->total_records();
  data.total_bytes = (*dataset)->total_bytes();
  DataSet value(data);

  planner_metadata_[dataset_path] = std::unique_ptr<DataSet>(new DataSet(value));
}
#endif
} // namespace pegasus