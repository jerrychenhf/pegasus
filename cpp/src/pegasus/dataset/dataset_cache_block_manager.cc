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

#include "pegasus/dataset/dataset_cache_block_manager.h"

using namespace pegasus;

namespace pegasus {

DatasetCacheBlockManager::DatasetCacheBlockManager() {
}

DatasetCacheBlockManager::~DatasetCacheBlockManager() {}

Status DatasetCacheBlockManager::GetCachedDataSet(Identity* identity, std::shared_ptr<CachedDataset>* dataset) {
  std::string dataset_path = identity->dataset_path();
  auto entry = cached_datasets_.find(dataset_path);

  if (entry == cached_datasets_.end()) {
    *dataset = NULL;
    return Status::OK(); 
    //   // Insert new record
    // std::shared_ptr<CachedDataset> new_dataset = std::shared_ptr<CachedDataset>(new CachedDataset(dataset_path));
    // cached_datasets_[dataset_path] = std::move(new_dataset);
  }
  auto find_cache_info = entry->second;
  *dataset = std::shared_ptr<CachedDataset>(find_cache_info);
  return Status::OK();
}


Status DatasetCacheBlockManager::GetCachedPartition(Identity* identity, std::shared_ptr<CachedPartition>* partition) {
  std::string dataset_path = identity->dataset_path();
  std::string partition_path = identity->file_path();
  std::shared_ptr<CachedDataset>* dataset;
  GetCachedDataSet(identity, dataset);
  auto entry = (*dataset)->cached_partitions_.find(partition_path);

  if (entry == (*dataset)->cached_partitions_.end()) {
    *partition = NULL;
    return Status::OK(); 
    // // Insert new record
    // std::shared_ptr<CachedPartition> new_partition = std::shared_ptr<CachedPartition>(new CachedPartition(dataset_path, partition_path));
    // (*dataset)->cached_partitions_[partition_path] = std::move(new_partition);
  }
  auto find_cache_info = entry->second;
  *partition = std::shared_ptr<CachedPartition>(find_cache_info);
  return Status::OK();
}

 Status DatasetCacheBlockManager::GetCachedColumns(Identity* identity, std::unordered_map<string, std::shared_ptr<CachedColumn>>* columns) {
  std::shared_ptr<CachedPartition>* partition;
  GetCachedPartition(identity, partition);
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
 }

// Status DatasetCacheBlockManager::InsertPartition(Identity* identity, std::shared_ptr<CachedPartition> new_partition) {
//   std::string dataset_path = identity->dataset_path();
//   std::shared_ptr<CachedDataset>* dataset;
//   GetCachedDataSet(identity, dataset);
//   string partition_path = identity->file_path();
//   (*dataset)->cached_partitions_[partition_path] = new_partition;
// }

Status DatasetCacheBlockManager::InsertColumn(Identity* identity, std::shared_ptr<CachedColumn> new_column) {
  // std::shared_ptr<CachedPartition>* partition;
  // GetCachedPartition(identity, partition);
  // int column_id = identity->num_rows();
  // (*partition)->cached_columns_[std::to_string(column_id)] = std::move(new_column);
}

} // namespace pegasus