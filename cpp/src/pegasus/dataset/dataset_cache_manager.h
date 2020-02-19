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

#include "arrow/record_batch.h"
#include "arrow/table.h"

#include "dataset/identity.h"
#include "dataset/dataset_cache_block_manager.h"
#include "dataset/dataset_cache_engine_manager.h"
#include "storage/storage_plugin.h"
#include "storage/storage_plugin_factory.h"
#include "rpc/server.h"

using namespace std;

namespace pegasus {

class DatasetCacheManager {
 public:
  DatasetCacheManager();
  ~DatasetCacheManager();
  
  Status Init();

  Status GetDatasetStream(Identity* identity, std::unique_ptr<rpc::FlightDataStream>* data_stream);
  
 private: 
  std::shared_ptr<DatasetCacheBlockManager> cache_block_manager_;
  std::shared_ptr<DatasetCacheEngineManager> cache_engine_manager_;
  std::shared_ptr<StoragePluginFactory> storage_plugin_factory_;
  
  CacheEngine::CachePolicy GetCachePolicy(Identity* identity);
  
  Status AddNewColumns(Identity* identity,
    std::unordered_map<string, std::shared_ptr<CachedColumn>> retrieved_columns);
  Status WrapDatasetStream(Identity* identity,
    std::unique_ptr<rpc::FlightDataStream>* data_stream);
  Status GetDatasetStreamWithMissedColumns(Identity* identity,
    std::vector<int> col_ids,
    std::unique_ptr<rpc::FlightDataStream>* data_stream);
  std::vector<int> GetMissedColumnsIds(std::vector<int> col_ids,
    std::unordered_map<string, std::shared_ptr<CachedColumn>> cached_columns);
    
  Status RetrieveColumns(Identity* identity,
    const std::vector<int>& col_ids,
    std::shared_ptr<CacheEngine> cache_engine,
    std::unordered_map<string, std::shared_ptr<CachedColumn>>& retrieved_columns
    );
};

} // namespace pegasus

#endif  // PEGASUS_DATASET_CACHE_MANAGER_H