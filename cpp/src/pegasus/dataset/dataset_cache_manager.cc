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

using namespace arrow;
namespace pegasus {

DatasetCacheManager::DatasetCacheManager(): dataset_cache_block_manager_(new DatasetCacheBlockManager()),
 dataset_cache_engine_manager_(new DatasetCacheEngineManager()) {
   ExecEnv* env =  ExecEnv::GetInstance();
   storage_plugin_factory_ = env->get_storage_plugin_factory();
}

DatasetCacheManager::~DatasetCacheManager() {
}

CacheEngine::CachePolicy DatasetCacheManager::GetCachePolicy(Identity* identity) {
  // TODO Choose the CachePolicy based on the data type in Identity
  return CacheEngine::CachePolicy::LRU;
}

Status GetColumnsFromStorage(Identity* identity, std::shared_ptr<StoragePluginFactory> storage_plugin_factory, std::vector<int> col_ids,
 std::unordered_map<string, std::shared_ptr<CachedColumn>>* get_columns) {
    std::string partition_path = identity->file_path();
    std::shared_ptr<StoragePlugin> storage_plugin;

    // Get the ReadableFile
    storage_plugin_factory->GetStoragePlugin(partition_path, &storage_plugin);
    std::shared_ptr<HdfsReadableFile> file;
    storage_plugin->GetReadableFile(partition_path, &file);

    // Read the columns from Table.
    arrow::MemoryPool* memory_pool = new DRAMMemoryPool();
    parquet::ArrowReaderProperties properties(new parquet::ArrowReaderProperties());
    std::unique_ptr<ParquetReader> parquet_reader(new ParquetReader(file, memory_pool, properties));
    for(auto iter = col_ids.begin(); iter != col_ids.end(); iter ++) {
      std::shared_ptr<arrow::ChunkedArray> chunked_out;
      parquet_reader->ReadColumnChunk(*iter, chunked_out);
      int64_t length = chunked_out->length();
      uint8_t** address = (uint8_t**)&chunked_out;
      std::shared_ptr<CacheRegion> cache_region = std::shared_ptr<CacheRegion>(new CacheRegion(address, length, length));
      std::shared_ptr<CachedColumn> column = std::shared_ptr<CachedColumn>(new CachedColumn(partition_path, *iter, cache_region));
      get_columns->insert(std::make_pair(std::to_string(*iter), column));
    }

    //  // Get cache engine.
    // std::shared_ptr<CacheEngine> cache_engine;
    // CacheEngine::CachePolicy cache_policy = DatasetCacheManager::GetCachePolicy(identity);
    // dataset_cache_engine_manager_->GetCacheEngine(cache_policy, &cache_engine);

    // Put the column value into cache engine.
    // int column_id = identity->num_rows();
    // CacheRegion cache_region;
    // cache_engine->PutValue(partition_path, column_id, cache_region);
    // // Insert the column info into dataset_cache_block_manager_.
    // std::shared_ptr<CachedColumn> column = std::shared_ptr<CachedColumn>(new CachedColumn(partition_path, column_id, cache_region));
    // dataset_cache_block_manager->InsertColumn(identity, column);
}

std::vector<int> GetUnCachedColumnsIds(std::vector<int> col_ids,  std::unordered_map<string, std::shared_ptr<CachedColumn>> cached_columns) {
   std::vector<int> uncached_columns;
    for(auto iter = col_ids.begin(); iter != col_ids.end(); iter ++) {
        auto entry = cached_columns.find(std::to_string(*iter));
        if (entry == cached_columns.end()) {
            uncached_columns.push_back(*iter);
        }
    }
    return uncached_columns;
}

Status InsertColumns(std::shared_ptr<DatasetCacheBlockManager> dataset_cache_block_manager_, std::unordered_map<string, std::shared_ptr<CachedColumn>> get_columns,
Identity* identity) {
  for(auto iter = get_columns.begin(); iter != get_columns.end(); iter ++) {
    dataset_cache_block_manager_->InsertColumn(identity, iter->first, iter->second);
  }
}

// Wrap the data to flight data stream.
// According DatasetCacheBlockManager to chech whether sotred.
// If yes, read it based on the address stored in DatasetCacheBlockManager and then return.
// If not, get dataset from hdfs and then put the dataset into CacheEngine.
//         1. Choose the CachePolicy based on the Identity.
//         2. Call DatasetCacheEngineManager#GetCacheEngine method to get CacheEngine;
Status DatasetCacheManager::GetDatasetStream(Identity* identity, std::unique_ptr<FlightDataStream>* data_stream) {
  // Check whether the dataset is cached.
  std::shared_ptr<CachedDataset> dataset;
  dataset_cache_block_manager_->GetCachedDataSet(identity, &dataset);
  std::vector<int> col_ids = identity->col_ids();
  std::unordered_map<string, std::shared_ptr<CachedColumn>> get_columns;
  if (dataset == NULL) {
    // Insert new dataset.
    std::shared_ptr<CachedDataset> new_dataset = std::shared_ptr<CachedDataset>(new CachedDataset(identity->dataset_path()));
    dataset_cache_block_manager_->InsertDataSet(identity, new_dataset);
    // Insert new partition.
    std::shared_ptr<CachedPartition> new_partition = std::shared_ptr<CachedPartition>(new CachedPartition(identity->dataset_path(), identity->file_path()));
    dataset_cache_block_manager_->InsertPartition(identity, new_partition);
    // dataset is not cached, get the dataset from the storage.
    GetColumnsFromStorage(identity, storage_plugin_factory_, col_ids, &get_columns);
    // Insert into the dataset_cache_block_manager_
    InsertColumns(dataset_cache_block_manager_, get_columns, identity);
  } else {
    // dataset is cached
    std::shared_ptr<CachedPartition> partition;
    dataset_cache_block_manager_->GetCachedPartition(identity, &partition);
    if (partition == NULL) {
      std::shared_ptr<CachedPartition> new_partition = std::shared_ptr<CachedPartition>(new CachedPartition(identity->dataset_path(), identity->file_path()));
      dataset_cache_block_manager_->InsertPartition(identity, new_partition);
      // partition is not cached, get the partition from the storage.
      GetColumnsFromStorage(identity, storage_plugin_factory_, col_ids, &get_columns);
      // Insert into the dataset_cache_block_manager_.
      InsertColumns(dataset_cache_block_manager_, get_columns, identity);
    } else {
      // partition is cached.
      // Check which column is cached.
      std::unordered_map<string, std::shared_ptr<CachedColumn>> cached_columns;
      dataset_cache_block_manager_->GetCachedColumns(identity, &cached_columns);
      if (col_ids.size() == cached_columns.size()) {
        // all columns cached.
      } else {
        // Not all columns cached.
        // Get the not cached col_ids.
        std::vector<int> uncached_col_ids = GetUnCachedColumnsIds(col_ids, cached_columns);
        // Get the uncached columns from HDFS.
        GetColumnsFromStorage(identity, storage_plugin_factory_, uncached_col_ids, &get_columns);
        // Insert into the dataset_cache_block_manager_.
        InsertColumns(dataset_cache_block_manager_, get_columns, identity);
      }
   }
  }
  //wrap the value to FlighrDataStream
}

} // namespace pegasus