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

#include "dataset/request_identity.h"
#include "dataset/cache_engine.h"
#include "cache/cache_region.h"
#include "arrow/table.h"

using namespace std;

namespace pegasus {

class CachedColumn {
 public:
  explicit CachedColumn(string partition_path, int column_id, CacheRegion* cache_region) :
  partition_path_(partition_path), column_id_(column_id), cache_region_(cache_region) {}

  ~CachedColumn();
 public:
  string partition_path_;
  int column_id_;
  
  // IMPORTANT: We owns the CacheRegion pointer
  // and delete it in destructor
  CacheRegion* cache_region_;
};

class CachedPartition {
 public:
  explicit CachedPartition(string dataset_path, string partition_path) :dataset_path_(dataset_path), partition_path_(partition_path){}

  string dataset_path_;
  string partition_path_;
  std::unordered_map<string, std::shared_ptr<CachedColumn>> cached_columns_;
};

class CachedDataset {
  public:
   explicit CachedDataset(string dataset_path): dataset_path_(dataset_path) {}

  public:
   string dataset_path_;
   std::unordered_map<std::string, std::shared_ptr<CachedPartition>> cached_partitions_;
};

class DatasetCacheBlockManager {
 public:
  DatasetCacheBlockManager();
  ~DatasetCacheBlockManager();
  
  Status Init();
  
  Status GetCachedDataSet(std::string dataset_path, std::shared_ptr<CachedDataset>* dataset);
  Status GetCachedPartition(std::string dataset_path, std::string partition_path,
   std::shared_ptr<CachedPartition>* partitios);
  Status GetCachedColumns(std::string dataset_path, std::string partition_path, std::vector<int> col_ids,
    std::unordered_map<string, std::shared_ptr<CachedColumn>>* cached_columns);

  Status InsertDataSet(std::string dataset_path, std::shared_ptr<CachedDataset> new_dataset);
  Status InsertPartition(std::string dataset_path, std::string partition_path,
   std::shared_ptr<CachedPartition> new_partition);
  Status InsertColumn(std::string dataset_path, std::string partition_path,
   string column_id, std::shared_ptr<CachedColumn> new_column);

  Status DeleteColumn(std::string dataset_path, std::string partition_path,
   string column_id);
 
 private: 
  std::unordered_map<std::string, std::shared_ptr<CachedDataset>> cached_datasets_;
};

} // namespace pegasus

#endif  // PEGASUS_DATASET_CACHE_BLOCK_MANAGER_H