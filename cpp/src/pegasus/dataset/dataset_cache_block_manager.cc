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

#include "dataset/dataset_cache_block_manager.h"

using namespace pegasus;

namespace pegasus {

DatasetCacheBlockManager::DatasetCacheBlockManager() {
}

DatasetCacheBlockManager::~DatasetCacheBlockManager() {}

Status DatasetCacheBlockManager::GetCachedDataSet(Identity* identity,
 std::shared_ptr<CachedDataset>* dataset) {
  std::string dataset_path = identity->dataset_path();
  auto entry = cached_datasets_.find(dataset_path);

  if (entry == cached_datasets_.end()) {
    *dataset = NULL;
    LOG(WARNING) << "The dataset: "<< identity->dataset_path() <<" is NULL.";
    return Status::OK(); 
  }
  auto find_cache_info = entry->second;
  *dataset = std::shared_ptr<CachedDataset>(find_cache_info);
  return Status::OK();
}


Status DatasetCacheBlockManager::GetCachedPartition(Identity* identity,
 std::shared_ptr<CachedPartition>* partition) {
  std::string dataset_path = identity->dataset_path();
  std::string partition_path = identity->file_path();
  std::shared_ptr<CachedDataset>* dataset;
  GetCachedDataSet(identity, dataset);
  if (dataset != NULL) {
    auto entry = (*dataset)->cached_partitions_.find(partition_path);

    if (entry == (*dataset)->cached_partitions_.end()) {
      *partition = NULL;
      LOG(WARNING) << "The partition: "<< identity->file_path() <<" is NULL.";
      return Status::OK(); 
    }
    auto find_cache_info = entry->second;
    *partition = std::shared_ptr<CachedPartition>(find_cache_info);
    return Status::OK();
  } else {
    stringstream ss;
    ss << "Can not get the partition: "<< identity->file_path() 
      <<"when the dataset: "<< identity->dataset_path() <<" is NULL";
    LOG(ERROR) << ss.str();
    return Status::UnknownError(ss.str());
  }
}

 Status DatasetCacheBlockManager::GetCachedColumns(Identity* identity,
  std::unordered_map<string, std::shared_ptr<CachedColumn>>* columns) {
  std::shared_ptr<CachedPartition>* partition;
  GetCachedPartition(identity, partition);
  if (partition != NULL) {
    std::vector<int> col_ids = identity->col_ids();
    for (auto iter = col_ids.begin(); iter != col_ids.end(); iter++)
  	{
		  auto entry = (*partition)->cached_columns_.find(std::to_string(*iter));
      if (entry != (*partition)->cached_columns_.end()) {
        auto find_column = entry->second;
        columns->insert(std::make_pair(std::to_string(*iter), find_column));
      }
	  }
    return Status::OK();
  } else {
    stringstream ss;
    ss << "Can not get the columns when the dataset: "<< identity->dataset_path() <<" is NULL";
    LOG(ERROR) << ss.str();
    return Status::UnknownError(ss.str());
  }
 }

Status DatasetCacheBlockManager::InsertDataSet(Identity* identity,
 std::shared_ptr<CachedDataset> new_dataset) {
  std::string dataset_path = identity->dataset_path();
  cached_datasets_.insert(std::make_pair(dataset_path, new_dataset));
}

Status DatasetCacheBlockManager::InsertPartition(Identity* identity,
 std::shared_ptr<CachedPartition> new_partition) {
  std::string dataset_path = identity->dataset_path();
  std::shared_ptr<CachedDataset>* dataset;
  GetCachedDataSet(identity, dataset);
  if (dataset != NULL) {
    string partition_path = identity->file_path();
    (*dataset)->cached_partitions_[partition_path] = new_partition;
  } else {
    stringstream ss;
    ss << "Can not insert new partition when the dataset: "<< identity->dataset_path() <<" is NULL";
    LOG(ERROR) << ss.str();
    return Status::UnknownError(ss.str());
  }
}

Status DatasetCacheBlockManager::InsertColumn(Identity* identity, string column_id,
 std::shared_ptr<CachedColumn> new_column) {
  std::shared_ptr<CachedPartition>* partition;
  GetCachedPartition(identity, partition);
  if (partition != NULL) {
    (*partition)->cached_columns_[column_id] = std::move(new_column);
  } else {
    stringstream ss;
    ss << "Can not insert new column when the partition: "<< identity->file_path() <<" is NULL";
    LOG(ERROR) << ss.str();
    return Status::UnknownError(ss.str());
  }
}

} // namespace pegasus