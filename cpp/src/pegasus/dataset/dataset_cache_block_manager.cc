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
#include <boost/thread/lock_guard.hpp>
#include "common/logging.h"

#include "cache/cache_engine.h"

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

Status DatasetCacheBlockManager::GetCachedDataSet(const std::string dataset_path,
 std::shared_ptr<CachedDataset>* dataset) {
 
  {
    boost::lock_guard<boost::mutex> l(cached_datasets_lock_);
    auto entry = cached_datasets_.find(dataset_path);

   if (entry == cached_datasets_.end()) {
    std::shared_ptr<CachedDataset> new_dataset =
     std::shared_ptr<CachedDataset>(new CachedDataset(dataset_path));
    *dataset = new_dataset;

    // Insert the dataset
    cached_datasets_.insert(std::make_pair(dataset_path, new_dataset));
    LOG(WARNING) << "The dataset does not exist and insert a new dataset";
    return Status::OK(); 
    }

   LOG(WARNING) << "The dataset exists and return the cached dataset";
   auto find_cache_info = entry->second;
   *dataset = std::shared_ptr<CachedDataset>(find_cache_info);
  }

  return Status::OK();
}


Status CachedDataset::GetCachedPartition(
 const std::string partition_path,
   std::shared_ptr<CachedPartition>* partition) {

   {
    boost::lock_guard<boost::mutex> l(cached_partitions_lock_);
    auto entry = cached_partitions_.find(partition_path);
  
    if (entry == cached_partitions_.end()) {
      std::shared_ptr<CachedPartition> new_partition =
      std::shared_ptr<CachedPartition>(new CachedPartition(
       dataset_path_, partition_path));
       if (new_partition == nullptr) {
         return Status::Invalid("The new partition is nullptr");
       }
      *partition = new_partition;

      // Insert partition
      cached_partitions_[partition_path] = new_partition;
      LOG(WARNING) << "The partition does not exist and insert a new partition";
      return Status::OK(); 
    }
    
    LOG(WARNING) << "The partition exists and return the cached partition";
    auto find_cache_info = entry->second;
    *partition = std::shared_ptr<CachedPartition>(find_cache_info);
   }
  return Status::OK();
}

 Status CachedPartition::GetCachedColumns(
   std::vector<int>  col_ids,
   std::shared_ptr<CacheEngine> cache_engine,
    unordered_map<int, std::shared_ptr<CachedColumn>>* columns) {
  {
    boost::lock_guard<boost::mutex> l(cached_columns_lock_); 
    std::string dataset_path = dataset_path_;
    std::string partition_path = partition_path_;

    for (auto iter = col_ids.begin(); iter != col_ids.end(); iter++)
    {
		  auto entry = cached_columns_.find(*iter);
      if (entry != cached_columns_.end()) {
        int colId = *iter;
        auto find_column = entry->second;
        int64_t column_size = find_column->GetCacheRegion()->size();

        LRUCache::CacheKey* key = new LRUCache::CacheKey(dataset_path, partition_path, colId, column_size);
        // Touch value in lru cache when access the cached column.
        cache_engine->TouchValue(key);

        columns->insert(pair<int, std::shared_ptr<CachedColumn>>(colId, find_column));
       }
	  }
  }
  return Status::OK();
 }

bool CachedPartition::InsertColumn(
   int column_id, std::shared_ptr<CachedColumn> new_column,
    std::shared_ptr<CachedColumn>* cached_column) {
  {
    boost::lock_guard<boost::mutex> l(cached_columns_lock_); 
    auto entry = cached_columns_.find(column_id);
    if(entry == cached_columns_.end()) {
      // insert new column
      LOG(WARNING) << "The column does not exist and insert new column";
      cached_columns_[column_id] = std::move(new_column);
      *cached_column = nullptr;
      return true;
    } else {
      // the column is already existed.
      LOG(WARNING) << "The column already exists and will not insert this column";
      *cached_column = entry->second;
      return false;
    }
  }
}

Status DatasetCacheBlockManager::DeleteDataset(const std::string dataset_path) {
  {
    boost::lock_guard<boost::mutex> l(cached_datasets_lock_);
    LOG(WARNING) << "Delete the dataset";
    cached_datasets_.erase(dataset_path);
  }
  return Status::OK();
}

Status CachedDataset::DeletePartition(const std::string partition_path){
  {
    boost::lock_guard<boost::mutex> l(cached_partitions_lock_); 
    LOG(WARNING) << "Delete the partition";
    cached_partitions_.erase(partition_path);
  }
  return Status::OK();
}

Status CachedPartition::DeleteColumn(int column_id) {
  {
    boost::lock_guard<boost::mutex> l(cached_columns_lock_);  
    LOG(WARNING) << "Delete the column"; 
    cached_columns_.erase(column_id);
  } 
  return Status::OK();
}

} // namespace pegasus
