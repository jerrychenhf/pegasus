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

namespace pegasus
{

DataSetStore::DataSetStore()
{
}

DataSetStore::~DataSetStore()
{
}

Status DataSetStore::GetDataSets(std::shared_ptr<std::vector<std::shared_ptr<DataSet>>> *datasets)
{
  boost::lock_guard<SpinLock> l(wholestore_lock_);
  datasets->get()->reserve(dataset_map_.size());
  for (auto entry : dataset_map_)
  {
    //TODO: lockread for each dataset
    datasets->get()->push_back(entry.second);
  }
  return Status::OK();
}

Status DataSetStore::GetDataSet(std::string dataset_path, std::shared_ptr<DataSet> *dataset)
{
  LOG(INFO) << "GetDataSet()...";
  boost::lock_guard<SpinLock> l(wholestore_lock_);
  auto entry = dataset_map_.find(dataset_path);
  if (entry == dataset_map_.end())
  {
    return Status::KeyError("Could not find dataset.", dataset_path);
  }
  else
  {
    *dataset = entry->second;
  }

  LOG(INFO) << "...GetDataSet()";
  return Status::OK();
}

Status DataSetStore::InsertDataSet(std::shared_ptr<DataSet> dataset)
{
  std::string key = dataset->dataset_path();
  boost::lock_guard<SpinLock> l(wholestore_lock_);
  dataset_map_[key] = std::move(dataset);
  return Status::OK();
}

Status DataSetStore::InvalidateAll()
{
  boost::lock_guard<SpinLock> l(wholestore_lock_);
  for (auto i : dataset_map_)
  {
    i.second->setRefreshFlag(DSRF_WORKERSET_CHG);
  }
  return Status::OK();
}

Status DataSetStore::UpdateDataSet(std::shared_ptr<DataSet> dataset)
{
  return Status::OK();
}

Status DataSetStore::RemoveDataSet(std::shared_ptr<DataSet> dataset)
{
  std::string key = dataset->dataset_path();
  boost::lock_guard<SpinLock> l(wholestore_lock_);
  dataset_map_.erase(key);
  return Status::OK();
}

} // namespace pegasus
