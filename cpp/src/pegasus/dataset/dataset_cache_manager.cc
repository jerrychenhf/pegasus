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

#include <memory>
#include <unordered_map>

#include "pegasus/dataset/dataset_cache_manager.h"
#include "pegasus/runtime/exec_env.h"
#include "pegasus/parquet/parquet_reader.h"
#include "pegasus/cache/memory_pool.h"


namespace pegasus {

DatasetCacheManager::DatasetCacheManager(): dataset_cache_block_manager_(new DatasetCacheBlockManager()),
 dataset_cache_engine_manager_(new DatasetCacheEngineManager()), dataset_cache_store_manager_(new DatasetCacheStoreManager()){
   ExecEnv* env =  ExecEnv::GetInstance();
   storage_plugin_factory_ = env->get_storage_plugin_factory();
}

DatasetCacheManager::~DatasetCacheManager() {
}

CacheEngine::CachePolicy DatasetCacheManager::GetCachePolicy(Identity* identity) {
  // TODO Choose the CachePolicy based on the data type in Identity
  return CacheEngine::CachePolicy::LRU;
}

// Wrap the data to flight data stream.
// According DatasetCacheBlockManager to chech whether sotred.
// If yes, read it based on the address stored in DatasetCacheBlockManager and then return.
// If not, get dataset from hdfs and then put the dataset into CacheEngine.
//         1. Choose the CachePolicy based on the Identity.
//         2. Call DatasetCacheEngineManager#GetCacheEngine method to get CacheEngine;
Status DatasetCacheManager::GetDatasetStream(Identity* identity, std::unique_ptr<FlightDataStream>* data_stream) {
  // Check whether the column in file is cached.
  // Identity# columns
  std::shared_ptr<CachedColumn> column;
  dataset_cache_block_manager_->GetCachedColumn(identity, &column);
  if (column == NULL) {
    // we need get from HDFS
    std::string partition_path = identity->file_path();

    // Get the Table from hdfs storage
    storage_plugin_factory_->GetStoragePlugin(partition_path, &storage_plugin_);
    std::shared_ptr<HdfsReadableFile> file;

    storage_plugin_->GetReadableFile(partition_path, &file);
    Store::StoreType store_type = dataset_cache_store_manager_->GetStorePolicy();

    std::shared_ptr<MemoryPool>* memory_pool;
    // dataset_cache_store_manager_->GetStoreMemoryPool(store_type, memory_pool);
    parquet::ArrowReaderProperties properties(new parquet::ArrowReaderProperties());

    // std::unique_ptr<ParquetReader> parquet_reader(new ParquetReader(file, memory_pool, properties));

    //  std::shared_ptr<arrow::Table> table;
    //  parquet_reader->ReadTable(table); // read all columns.

    // put the columns into cache engine.
    std::shared_ptr<CacheEngine> cache_engine;
    CacheEngine::CachePolicy cache_policy = GetCachePolicy(identity);
    dataset_cache_engine_manager_->GetCacheEngine(cache_policy, &cache_engine);
    int column_id = identity->num_rows();
    CacheEntryHolder cache_entry_holder;
    cache_engine->PutValue(partition_path, column_id, cache_entry_holder);
    std::shared_ptr<CachedColumn> column = std::shared_ptr<CachedColumn>(new CachedColumn(partition_path, column_id, cache_entry_holder));
    dataset_cache_block_manager_->InsertColumn(identity, column);
  }

   //wrap the value to FlighrDataStream
}

} // namespace pegasus