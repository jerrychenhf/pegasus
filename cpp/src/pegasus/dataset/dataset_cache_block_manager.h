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

#ifndef PEGASUS_DATASET_CACHE_BLOCK_MANAGER_H
#define PEGASUS_DATASET_CACHE_BLOCK_MANAGER_H

#include <unordered_map>
#include <boost/thread/mutex.hpp>

#include "dataset/request_identity.h"
#include "cache/cache_region.h"

using namespace std;
using std::string;

namespace pegasus {

struct DatasetKey {
 public:
  explicit DatasetKey(const std::string& identity) : identity_(identity) {}
  std::size_t Hash() const {
    std::hash<std::string> h;
     return h(identity_);
  }
  bool operator==(const DatasetKey& other) const { return identity_ == other.identity_; }

 private:
  const std::string& identity_;
};

struct hasher {
    std::size_t operator()(const DatasetKey& i) const {
      return i.Hash();
    }
};

class CachedColumn {
 public:
  explicit CachedColumn(std::string partition_path, int column_id, CacheRegion* cache_region) :
  partition_path_(partition_path), column_id_(column_id), cache_region_(cache_region) {}

  ~CachedColumn();

  std::string GetPartitionPath() { return partition_path_; }
  int GetColumnId() { return column_id_; }
  CacheRegion* GetCacheRegion() {
    return cache_region_;
  }

 private:
  std::string partition_path_;
  int column_id_;
  
  // IMPORTANT: We owns the CacheRegion pointer
  // and delete it in destructor
  CacheRegion* cache_region_;
};

class CachedPartition {
 public:
  explicit CachedPartition(std::string dataset_path,
   std::string partition_path) :dataset_path_(dataset_path), partition_path_(partition_path){}
  
  Status GetCachedColumns(std::shared_ptr<CachedPartition> cached_partition, std::vector<int>  col_ids,
    unordered_map<int, std::shared_ptr<CachedColumn>>* cached_columns);
  bool InsertColumn(std::shared_ptr<CachedPartition> cached_partition,
   int column_id, std::shared_ptr<CachedColumn> new_column, std::shared_ptr<CachedColumn>* cached_column);
  Status DeleteColumn(std::shared_ptr<CachedPartition> cached_partition, int column_id);

  std::string GetDatasetPath() { return dataset_path_; }
  std::string GetPartitionPath() {return partition_path_; }
  unordered_map<int, std::shared_ptr<CachedColumn>> GetCachedColumns() {
    return cached_columns_;
  }

 private:
  std::string dataset_path_;
  std::string partition_path_;
  boost::mutex cached_columns_lock_;
  unordered_map<int, std::shared_ptr<CachedColumn>> cached_columns_;
};

class CachedDataset {
  public:
   explicit CachedDataset(std::string dataset_path): dataset_path_(dataset_path) {}

  Status GetCachedPartition(std::shared_ptr<CachedDataset> cached_dataset, std::string partition_path,
   std::shared_ptr<CachedPartition>* partition);


  Status DeletePartition(std::shared_ptr<CachedDataset> cached_dataset, std::string partition_path);
  
  std::string GetDatasetPath() { return dataset_path_; }
  std::unordered_map<string, std::shared_ptr<CachedPartition>> GetCachedPartitions() {
    return cached_partitions_;
  }

  private:
   std::string dataset_path_;
   boost::mutex cached_partitions_lock_;
   std::unordered_map<string, std::shared_ptr<CachedPartition>> cached_partitions_;
};

class DatasetCacheBlockManager {
 public:
  DatasetCacheBlockManager();
  ~DatasetCacheBlockManager();
  
  Status Init();
  
  Status GetCachedDataSet(std::string dataset_path, std::shared_ptr<CachedDataset>* dataset);

  Status DeleteDataset(std::string dataset_path);

  std::unordered_map<string, std::shared_ptr<CachedDataset>> GetCachedDatasets() {
    return cached_datasets_;
  }

 private: 
  boost::mutex cached_datasets_lock_;
  std::unordered_map<string, std::shared_ptr<CachedDataset>> cached_datasets_;
};

} // namespace pegasus

#endif  // PEGASUS_DATASET_CACHE_BLOCK_MANAGER_H