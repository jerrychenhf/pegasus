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

class CacheEngine;

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
  explicit CachedColumn(const std::string partition_path, int column_id, CacheRegion* cache_region) :
  partition_path_(partition_path), column_id_(column_id), cache_region_(cache_region) {}

  ~CachedColumn();

  const std::string GetPartitionPath() { return partition_path_; }
  int GetColumnId() { return column_id_; }
  CacheRegion* GetCacheRegion() {
    return cache_region_;
  }

 private:
  const std::string partition_path_;
  int column_id_;
  
  // IMPORTANT: We owns the CacheRegion pointer
  // and delete it in destructor
  CacheRegion* cache_region_;
};

class CachedPartition {
 public:
  explicit CachedPartition(const std::string dataset_path,
   const std::string partition_path) :dataset_path_(dataset_path), partition_path_(partition_path){}
  
  Status GetCachedColumns(std::vector<int>  col_ids,
    std::shared_ptr<CacheEngine> cache_engine,
    unordered_map<int, std::shared_ptr<CachedColumn>>* cached_columns);
  bool InsertColumn(
   int column_id, std::shared_ptr<CachedColumn> new_column, std::shared_ptr<CachedColumn>* cached_column);
  Status DeleteColumn(int column_id);
  Status DeleteEntry(std::shared_ptr<CacheEngine> cache_engine);

  const std::string GetDatasetPath() { return dataset_path_; }
  const std::string GetPartitionPath() {return partition_path_; }
  unordered_map<int, std::shared_ptr<CachedColumn>>& GetCachedColumns() {
    return cached_columns_;
  }

 private:
  const std::string dataset_path_;
  const std::string partition_path_;
  boost::mutex cached_columns_lock_;
  unordered_map<int, std::shared_ptr<CachedColumn>> cached_columns_;
};

class CachedDataset {
  public:
   explicit CachedDataset(const std::string dataset_path): dataset_path_(dataset_path) {}

  Status GetCachedPartition(const std::string partition_path,
   std::shared_ptr<CachedPartition>* partition);


  Status DeletePartition(const std::string partition_path, std::shared_ptr<CacheEngine> cache_engine);
  
  const std::string& GetDatasetPath() { return dataset_path_; }
  std::unordered_map<string, std::shared_ptr<CachedPartition>>& GetCachedPartitions() {
    return cached_partitions_;
  }

  private:
   const std::string dataset_path_;
   boost::mutex cached_partitions_lock_;
   std::unordered_map<string, std::shared_ptr<CachedPartition>> cached_partitions_;
};

class DatasetCacheBlockManager {
 public:
  DatasetCacheBlockManager();
  ~DatasetCacheBlockManager();
  
  Status Init();
  
  Status GetCachedDataSet(const std::string dataset_path,
    std::shared_ptr<CachedDataset>* dataset);

  Status DeleteDataset(const std::string dataset_path);

  const std::unordered_map<string, std::shared_ptr<CachedDataset>>& GetCachedDatasets() {
    return cached_datasets_;
  }

 private: 
  boost::mutex cached_datasets_lock_;
  std::unordered_map<string, std::shared_ptr<CachedDataset>> cached_datasets_;
};

} // namespace pegasus

#endif  // PEGASUS_DATASET_CACHE_BLOCK_MANAGER_H
