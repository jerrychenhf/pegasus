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

#ifndef PEGASUS_DATASET_CACHE_MANAGER_H
#define PEGASUS_DATASET_CACHE_MANAGER_H

#include "dataset/dataset_cache_block_manager.h"
#include "dataset/dataset_cache_engine_manager.h"
#include "storage/storage.h"
#include "storage/storage_factory.h"
#include "rpc/server.h"
#include "dataset/request_identity.h"
#include "rpc/types.h"

using namespace std;
using std::string;

namespace arrow {
  class ChunkedArray;
  class Table;
}

namespace pegasus {

class DatasetCacheManager {
 public:
  DatasetCacheManager();
  ~DatasetCacheManager();
  
  Status Init();

  Status GetDatasetStream(RequestIdentity* request_identity, std::unique_ptr<rpc::FlightDataStream>* data_stream);

  Status DropCachedDataset(std::vector<rpc::PartsDropListofDataset> drop_lists);

 public:
  DatasetCacheEngineManager* cache_engine_manager_;
  DatasetCacheBlockManager* cache_block_manager_;
 private: 
  
  std::shared_ptr<StorageFactory> storage_factory_;
  
  CacheEngine::CachePolicy GetCachePolicy(RequestIdentity* request_identity);
  
  Status WrapDatasetStream(RequestIdentity* request_identity,
  unordered_map<int, std::shared_ptr<CachedColumn>> columns,
    std::unique_ptr<rpc::FlightDataStream>* data_stream);
  Status GetDatasetStreamWithMissedColumns(RequestIdentity* request_identity,
    std::vector<int> col_ids,
    unordered_map<int, std::shared_ptr<CachedColumn>> cached_columns,
    std::shared_ptr<CacheEngine> cache_engine,
    std::unique_ptr<rpc::FlightDataStream>* data_stream);
  std::vector<int> GetMissedColumnsIds(std::vector<int> col_ids,
    unordered_map<int, std::shared_ptr<CachedColumn>> cached_columns);
    
  Status RetrieveColumns(RequestIdentity* request_identity,
    const std::vector<int>& col_ids,
    std::shared_ptr<CacheEngine> cache_engine,
    unordered_map<int, std::shared_ptr<CachedColumn>>& retrieved_columns
    );
  Status GetPartition(RequestIdentity* request_identity,
    std::shared_ptr<CachedPartition>* new_partition);
};

} // namespace pegasus

#endif  // PEGASUS_DATASET_CACHE_MANAGER_H