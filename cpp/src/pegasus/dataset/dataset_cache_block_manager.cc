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
#include "common/logging.h"

using namespace pegasus;

namespace pegasus {
  
CachedColumn::~CachedColumn() {
  if(cache_region_ != nullptr) {
    delete cache_region_;
    cache_region_ = nullptr;
  }
}

DatasetCacheBlockManager::DatasetCacheBlockManager() {
}

DatasetCacheBlockManager::~DatasetCacheBlockManager() {
}

Status DatasetCacheBlockManager::Init() {   
  return Status::OK();
}

Status DatasetCacheBlockManager::GetCachedDataSet(std::string dataset_path,
 std::shared_ptr<CachedDataset>* dataset) {
  if (cached_datasets_.size() == 0) {
     *dataset = nullptr;
    LOG(WARNING) << "The dataset: "<< dataset_path <<" is NULL.";
    return Status::OK(); 
  }
  auto entry = cached_datasets_.find(dataset_path);

  if (entry == cached_datasets_.end()) {
    *dataset = nullptr;
    LOG(WARNING) << "The dataset: "<< dataset_path <<" is NULL.";
    return Status::OK(); 
  }
  auto find_cache_info = entry->second;
  *dataset = std::shared_ptr<CachedDataset>(find_cache_info);
  return Status::OK();
}


Status DatasetCacheBlockManager::GetCachedPartition(std::string dataset_path,
  std::string partition_path, std::shared_ptr<CachedPartition>* partition) {
  std::shared_ptr<CachedDataset> dataset;
  RETURN_IF_ERROR(GetCachedDataSet(dataset_path, &dataset));
  if (dataset == nullptr) {
    stringstream ss;
    ss << "Can not get the partition: "<< partition_path 
      <<"when the dataset: "<< dataset_path <<" is NULL";
    LOG(ERROR) << ss.str();
    return Status::UnknownError(ss.str());
  }

  auto entry = dataset->cached_partitions_.find(partition_path);
  if (entry == dataset->cached_partitions_.end()) {
    *partition = nullptr;
    LOG(WARNING) << "The partition: "<< partition_path <<" is NULL.";
    return Status::OK(); 
  }

  auto find_cache_info = entry->second;
  *partition = std::shared_ptr<CachedPartition>(find_cache_info);
  return Status::OK();
}

 Status DatasetCacheBlockManager::GetCachedColumns(std::string dataset_path,
  std::string partition_path, std::vector<int> col_ids,
  std::unordered_map<string, std::shared_ptr<CachedColumn>>* columns) {
  std::shared_ptr<CachedPartition> partition;
  RETURN_IF_ERROR(GetCachedPartition(dataset_path, partition_path, &partition));
  if (partition == nullptr) {
    stringstream ss;
    ss << "Can not get the columns when the dataset: "<< dataset_path <<" is NULL";
    LOG(ERROR) << ss.str();
    return Status::UnknownError(ss.str());
  }
   
  for (auto iter = col_ids.begin(); iter != col_ids.end(); iter++)
  {
		auto entry = partition->cached_columns_.find(std::to_string(*iter));
    if (entry != partition->cached_columns_.end()) {
      auto find_column = entry->second;
      columns->insert(std::make_pair(std::to_string(*iter), find_column));
    }
	}
  return Status::OK();
 }

Status DatasetCacheBlockManager::InsertDataSet(std::string dataset_path,
 std::shared_ptr<CachedDataset> new_dataset) {
  cached_datasets_.insert(std::make_pair(dataset_path, new_dataset));
  return Status::OK();
}

Status DatasetCacheBlockManager::InsertPartition(std::string dataset_path,
  std::string partition_path, std::shared_ptr<CachedPartition> new_partition) {
  std::shared_ptr<CachedDataset> dataset;
  RETURN_IF_ERROR(GetCachedDataSet(dataset_path, &dataset));
  if (dataset == nullptr) {
    stringstream ss;
    ss << "Can not insert new partition when the dataset: "<< dataset_path <<" is NULL";
    LOG(ERROR) << ss.str();
    return Status::UnknownError(ss.str());
  } 
  dataset->cached_partitions_[partition_path] = new_partition;
  return Status::OK();
}

Status DatasetCacheBlockManager::InsertColumn(std::string dataset_path,
  std::string partition_path, string column_id,
 std::shared_ptr<CachedColumn> new_column) {
  std::shared_ptr<CachedPartition> partition;
  RETURN_IF_ERROR(GetCachedPartition(dataset_path, partition_path, &partition));
  if (partition == nullptr) {
    stringstream ss;
    ss << "Can not insert new column when the partition: "<< partition_path <<" is NULL";
    LOG(ERROR) << ss.str();
    return Status::UnknownError(ss.str());
  } 

  partition->cached_columns_[column_id] = std::move(new_column);
  return Status::OK();
}

Status DatasetCacheBlockManager::DeleteColumn(std::string dataset_path, std::string partition_path,
   string column_id) {
 std::shared_ptr<CachedPartition> partition;
 RETURN_IF_ERROR(GetCachedPartition(dataset_path, partition_path, &partition));

 if (partition == nullptr) {
    stringstream ss;
    ss << "Can not delete this column when the partition: "<< partition_path <<" is NULL";
    LOG(ERROR) << ss.str();
    return Status::UnknownError(ss.str());
 } 

 partition->cached_columns_.erase(column_id);
 
 return Status::OK();
}

} // namespace pegasus