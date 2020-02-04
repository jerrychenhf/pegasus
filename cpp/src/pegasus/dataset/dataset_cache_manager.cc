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

#include "dataset/dataset_cache_manager.h"
#include "runtime/exec_env.h"
#include "parquet/parquet_reader.h"
#include "cache/memory_pool.h"
#include "util/logging.h"

namespace pegasus {

DatasetCacheManager::DatasetCacheManager() {
   dataset_cache_block_manager_ = std::shared_ptr<DatasetCacheBlockManager>(
     new DatasetCacheBlockManager());
   dataset_cache_engine_manager_ = std::shared_ptr<DatasetCacheEngineManager>
   (new DatasetCacheEngineManager());
   ExecEnv* env =  ExecEnv::GetInstance();
   storage_plugin_factory_ = env->get_storage_plugin_factory();
}

DatasetCacheManager::~DatasetCacheManager() {
}

CacheEngine::CachePolicy GetCachePolicy(Identity* identity) {
  // TODO Choose the CachePolicy based on the data type in Identity
  return CacheEngine::CachePolicy::LRU;
}

Status GetAndInsertColumns(Identity* identity, std::shared_ptr<StoragePluginFactory> storage_plugin_factory,
 std::vector<int> col_ids, std::shared_ptr<DatasetCacheBlockManager> dataset_cache_block_manager) {
    std::string partition_path = identity->file_path();
    std::shared_ptr<StoragePlugin> storage_plugin;

    // Get the ReadableFile  DEBUG CHECK
    DCHECK(storage_plugin_factory->GetStoragePlugin(partition_path, &storage_plugin).ok());
    std::shared_ptr<HdfsReadableFile> file;
    DCHECK(storage_plugin->GetReadableFile(partition_path, &file).ok());

    // Read the columns into ChunkArray.
    arrow::MemoryPool* memory_pool = new DRAMMemoryPool();
    parquet::ArrowReaderProperties properties(new parquet::ArrowReaderProperties());
    std::unique_ptr<ParquetReader> parquet_reader(new ParquetReader(file, memory_pool, properties));
    std::unordered_map<string, std::shared_ptr<CachedColumn>> get_columns;
    for(auto iter = col_ids.begin(); iter != col_ids.end(); iter ++) {
      std::shared_ptr<arrow::ChunkedArray> chunked_out;
      DCHECK(parquet_reader->ReadColumnChunk(*iter, chunked_out).ok());
      std::shared_ptr<CachedColumn> column = std::shared_ptr<CachedColumn>(
        new CachedColumn(partition_path, *iter, chunked_out));
      get_columns.insert(std::make_pair(std::to_string(*iter), column));
    }
    // Before insert into the column, check whether the dataset is inserted.
    std::shared_ptr<CachedDataset> dataset;
    DCHECK(dataset_cache_block_manager->GetCachedDataSet(identity, &dataset).ok());
    if (dataset == NULL) {
      // Insert new dataset.
      std::shared_ptr<CachedDataset> new_dataset = std::shared_ptr<CachedDataset>(
        new CachedDataset(identity->dataset_path()));
      DCHECK(dataset_cache_block_manager->InsertDataSet(identity, new_dataset).ok());

    }
    // After check the dataset, continue to check the partition is inserted.
    std::shared_ptr<CachedPartition> partition;
    DCHECK(dataset_cache_block_manager->GetCachedPartition(identity, &partition).ok());
    if (partition == NULL) {
      std::shared_ptr<CachedPartition> new_partition = std::shared_ptr<CachedPartition>(
        new CachedPartition(identity->dataset_path(), identity->file_path()));
      DCHECK(dataset_cache_block_manager->InsertPartition(identity, new_partition).ok());
    }

    // Insert the columns into dataset_cache_block_manager.
    for(auto iter = get_columns.begin(); iter != get_columns.end(); iter ++) {
      DCHECK(dataset_cache_block_manager->InsertColumn(identity, iter->first, iter->second).ok());
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

std::vector<int> GetUnCachedColumnsIds(std::vector<int> col_ids,
  std::unordered_map<string, std::shared_ptr<CachedColumn>> cached_columns) {
   std::vector<int> uncached_columns;
    for(auto iter = col_ids.begin(); iter != col_ids.end(); iter ++) {
        auto entry = cached_columns.find(std::to_string(*iter));
        if (entry == cached_columns.end()) {
            uncached_columns.push_back(*iter);
        }
    }
    return uncached_columns;
}

Status WrapDatasetStream(std::unique_ptr<rpc::FlightDataStream>* data_stream,
 std::shared_ptr<DatasetCacheBlockManager> dataset_cache_block_manager_, Identity* identity) {

  std::unordered_map<string, std::shared_ptr<CachedColumn>> cached_columns;
  DCHECK(dataset_cache_block_manager_->GetCachedColumns(identity, &cached_columns).ok());

  std::shared_ptr<Table> table;
  for(auto iter = cached_columns.begin(); iter != cached_columns.end(); iter ++) {
    std::shared_ptr<CachedColumn> cache_column = iter->second;
    std::shared_ptr<arrow::ChunkedArray> chunked_out = cache_column->chunked_array_;
    DCHECK(Table::FromChunkedStructArray(chunked_out, &table).ok());
  }
  *data_stream = std::unique_ptr<rpc::FlightDataStream>(
    new rpc::RecordBatchStream(std::shared_ptr<RecordBatchReader>(new TableBatchReader(*table))));
}

Status GetAndInsertAndWrapDataStream(Identity* identity, std::shared_ptr<StoragePluginFactory> storage_plugin_factory,
 std::vector<int> col_ids, std::shared_ptr<DatasetCacheBlockManager> dataset_cache_block_manager,
  std::unique_ptr<rpc::FlightDataStream>* data_stream) {
    DCHECK(GetAndInsertColumns(identity, storage_plugin_factory, col_ids, dataset_cache_block_manager).ok());
    DCHECK(WrapDatasetStream(data_stream, dataset_cache_block_manager, identity).ok());
}

// Wrap the data to flight data stream.
// According DatasetCacheBlockManager to chech whether sotred.
// If yes, read it based on the address stored in DatasetCacheBlockManager and then return.
// If not, get dataset from hdfs and then put the dataset into CacheEngine.
//         1. Choose the CachePolicy based on the Identity.
//         2. Call DatasetCacheEngineManager#GetCacheEngine method to get CacheEngine;
Status DatasetCacheManager::GetDatasetStream(Identity* identity,
 std::unique_ptr<rpc::FlightDataStream>* data_stream) {
  // Check whether the dataset is cached.
  std::shared_ptr<CachedDataset> dataset;
  dataset_cache_block_manager_->GetCachedDataSet(identity, &dataset);
  std::vector<int> col_ids = identity->col_ids();
  std::unordered_map<string, std::shared_ptr<CachedColumn>> get_columns;
  if (dataset == NULL) {
    LOG(WARNING) << "The dataset "<< identity->dataset_path() 
    <<" is NULL. We will get all the columns from storage and then insert the column into dataset cache block manager";
    GetAndInsertAndWrapDataStream(identity, storage_plugin_factory_, col_ids,
     dataset_cache_block_manager_, data_stream);
  } else {
    // dataset is cached
    std::shared_ptr<CachedPartition> partition;
    dataset_cache_block_manager_->GetCachedPartition(identity, &partition);
    if (partition == NULL) {
      LOG(WARNING) << "The partition "<< identity->file_path() 
      <<" is NULL. We will get all the columns from storage and then insert the column into dataset cache block manager";
      GetAndInsertAndWrapDataStream(identity, storage_plugin_factory_, col_ids,
       dataset_cache_block_manager_, data_stream);
    } else {
      // partition is cached.
      // Check which column is cached.
      std::unordered_map<string, std::shared_ptr<CachedColumn>> cached_columns;
      dataset_cache_block_manager_->GetCachedColumns(identity, &cached_columns);
      if (col_ids.size() == cached_columns.size()) {
        LOG(WARNING) << "All the columns are cached. And we will wrap the columns into Flight data stream";
        WrapDatasetStream(data_stream, dataset_cache_block_manager_, identity);
      } else {
        // Not all columns cached.
        // Get the not cached col_ids.
        std::vector<int> uncached_col_ids = GetUnCachedColumnsIds(col_ids, cached_columns);
        GetAndInsertAndWrapDataStream(identity, storage_plugin_factory_, uncached_col_ids,
         dataset_cache_block_manager_, data_stream);
      }
   }
  }
}

} // namespace pegasus