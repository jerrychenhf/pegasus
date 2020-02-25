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
  // if (cached_datasets_.size() != 0) {
  //   for(auto iter = cached_datasets_.begin(); iter != cached_datasets_.end(); iter++) {
  //     std::string dataset_path = iter->first;
  //     std::shared_ptr<CachedDataset> cached_dataset = iter->second;
  //     std::unordered_map<std::string, std::shared_ptr<CachedPartition>> cached_partitions =
  //      cached_dataset->cached_partitions_;
  //     if(cached_partitions.size() != 0) {
  //       for(auto iter = cached_partitions.begin(); iter != cached_partitions.end(); iter++) {
  //           std::string partition_path = iter->first;
  //           std::shared_ptr<CachedPartition> cached_partition = iter->second;
  //           std::unordered_map<string, std::shared_ptr<CachedColumn>> cached_columns = cached_partition->cached_columns_;
  //           if (cached_columns.size() != 0) {
  //             for(auto iter = cached_columns.begin(); iter != cached_columns.end(); iter ++) {
  //               std::string column_id = iter->first;
  //               DeleteColumn(dataset_path, partition_path, column_id);
  //             }
  //           }
  //           DeletePartition(dataset_path, partition_path);
  //       }
  //     }
  //     DeleteDataset(dataset_path);
  //   } 
  // }
}

Status DatasetCacheBlockManager::Init() {   
  return Status::OK();
}

Status DatasetCacheBlockManager::GetCachedDataSet(const std::string& dataset_path,
 std::shared_ptr<CachedDataset>* dataset) {
  if (cached_datasets_.size() == 0) {
     *dataset = nullptr;
    LOG(WARNING) << "The dataset: "<< dataset_path <<" is NULL.";
    return Status::OK(); 
  }
  DatasetKey key(dataset_path);
  auto entry = cached_datasets_.find(key);

  if (entry == cached_datasets_.end()) {
    *dataset = nullptr;
    LOG(WARNING) << "The dataset: "<< dataset_path <<" is NULL.";
    return Status::OK(); 
  }
  auto find_cache_info = entry->second;
  *dataset = std::shared_ptr<CachedDataset>(find_cache_info);
  return Status::OK();
}


Status DatasetCacheBlockManager::GetCachedPartition(const std::string& dataset_path,
  const std::string& partition_path, std::shared_ptr<CachedPartition>* partition) {
  std::shared_ptr<CachedDataset> dataset;
  RETURN_IF_ERROR(GetCachedDataSet(dataset_path, &dataset));
  if (dataset == nullptr) {
    stringstream ss;
    ss << "Can not get the partition: "<< partition_path 
      <<"when the dataset: "<< dataset_path <<" is NULL";
    LOG(ERROR) << ss.str();
    return Status::UnknownError(ss.str());
  }
  DatasetKey key(partition_path);
  auto entry = dataset->cached_partitions_.find(key);
  if (entry == dataset->cached_partitions_.end()) {
    *partition = nullptr;
    LOG(WARNING) << "The partition: "<< partition_path <<" is NULL.";
    return Status::OK(); 
  }

  auto find_cache_info = entry->second;
  *partition = std::shared_ptr<CachedPartition>(find_cache_info);
  return Status::OK();
}

 Status DatasetCacheBlockManager::GetCachedColumns(const std::string& dataset_path,
 const std::string& partition_path, std::vector<int> col_ids,
  unordered_map<int, std::shared_ptr<CachedColumn>>* columns) {
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
		auto entry = partition->cached_columns_.find(*iter);
    if (entry != partition->cached_columns_.end()) {
      auto find_column = entry->second;
      columns->insert(pair<int, std::shared_ptr<CachedColumn>>(*iter, find_column));
    }
	}
  return Status::OK();
 }

Status DatasetCacheBlockManager::InsertDataSet(const std::string& dataset_path,
 std::shared_ptr<CachedDataset> new_dataset) {
    DatasetKey key(dataset_path);
  cached_datasets_.insert(std::make_pair(key, new_dataset));
  return Status::OK();
}

Status DatasetCacheBlockManager::InsertPartition(const std::string& dataset_path,
  const std::string& partition_path, std::shared_ptr<CachedPartition> new_partition) {
  std::shared_ptr<CachedDataset> dataset;
  RETURN_IF_ERROR(GetCachedDataSet(dataset_path, &dataset));
  if (dataset == nullptr) {
    stringstream ss;
    ss << "Can not insert new partition when the dataset: "<< dataset_path <<" is NULL";
    LOG(ERROR) << ss.str();
    return Status::UnknownError(ss.str());
  } 
  DatasetKey key(partition_path);
  dataset->cached_partitions_[key] = new_partition;
  return Status::OK();
}

Status DatasetCacheBlockManager::InsertColumn(const std::string& dataset_path,
  const std::string& partition_path, int column_id,
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

Status DatasetCacheBlockManager::DeleteDataset(const std::string& dataset_path) {
   DatasetKey key(dataset_path);
  cached_datasets_.erase(key);
  return Status::OK();

}

Status DatasetCacheBlockManager::DeletePartition(const std::string& dataset_path, const std::string& partition_path){
  std::shared_ptr<CachedDataset> dataset;
  RETURN_IF_ERROR(GetCachedDataSet(dataset_path, &dataset));
  DatasetKey key(partition_path); 
  dataset->cached_partitions_.erase(key);
  return Status::OK();
}

Status DatasetCacheBlockManager::DeleteColumn(const std::string& dataset_path, const std::string& partition_path,
   int column_id) {
  std::shared_ptr<CachedPartition> partition;
  RETURN_IF_ERROR(GetCachedPartition(dataset_path, partition_path, &partition));

  if (partition == nullptr) {
    stringstream ss;
    ss << "Can not delete this column when the partition: "<< partition_path <<" is NULL";
    LOG(ERROR) << ss.str();
    return Status::UnknownError(ss.str());
  } 
  
  partition->cached_columns_.erase(0);
 
  return Status::OK();
}

} // namespace pegasus