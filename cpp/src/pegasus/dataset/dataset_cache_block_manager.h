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

#include "pegasus/dataset/identity.h"
#include "pegasus/cache/store_manager.h"
#include "pegasus/dataset/cache_engine.h"

using namespace std;

namespace pegasus {

class CachedColumn {
 public:
  explicit CachedColumn(int column_id, std::string address) :
  column_id_(column_id), address_(address) {}

 private:
  int column_id_;
  std::string address_;
};

class CachedPartition {
 public:
  explicit CachedPartition(string partition_path) : partition_path_(partition_path){}

  string partition_path_;
  std::vector<CachedColumn> columns_;
};

class CachedDataset {
  public:
   explicit CachedDataset(string dataset_path): dataset_path_(dataset_path_) {}
  std::shared_ptr<std::vector<CachedPartition>> partitions() const { return partitions_; }

  public:
   string dataset_path_;
   std::shared_ptr<std::vector<CachedPartition>> partitions_;
};

class DatasetCacheBlockManager {
 public:
  DatasetCacheBlockManager();
  ~DatasetCacheBlockManager();
  Status GetCachedDataSet(Identity* identity, std::shared_ptr<CachedDataset>* dataset);
  Status GetCachedPartition(Identity* identity, std::shared_ptr<std::vector<CachedPartition>>* partitions);
  Status InsertPartition(Identity* identity, std::shared_ptr<CachedPartition> new_partition);
 
 private: 
  std::unordered_map<std::string, std::shared_ptr<CachedDataset>> cached_datasets_;
};

} // namespace pegasus

#endif  // PEGASUS_DATASET_CACHE_BLOCK_MANAGER_H