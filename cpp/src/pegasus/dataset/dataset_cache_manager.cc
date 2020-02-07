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
#include "cache/cache_memory_pool.h"
#include "common/logging.h"

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

Status InsertColumnsToBlockManager(Identity* identity,
 std::shared_ptr<DatasetCacheBlockManager> dataset_cache_block_manager, 
 std::unordered_map<string, std::shared_ptr<CachedColumn>> retrieved_columns) {
    // Before insert into the column, check whether the dataset is inserted.
    std::shared_ptr<CachedDataset> dataset;
    RETURN_IF_ERROR(dataset_cache_block_manager->GetCachedDataSet(identity, &dataset));
    if (dataset == NULL) {
      // Insert new dataset.
      std::shared_ptr<CachedDataset> new_dataset = std::shared_ptr<CachedDataset>(
        new CachedDataset(identity->dataset_path()));
      RETURN_IF_ERROR(dataset_cache_block_manager->InsertDataSet(identity, new_dataset));

    }
    // After check the dataset, continue to check whether the partition is inserted.
    std::shared_ptr<CachedPartition> partition;
    RETURN_IF_ERROR(dataset_cache_block_manager->GetCachedPartition(identity, &partition));
    if (partition == NULL) {
      std::shared_ptr<CachedPartition> new_partition = std::shared_ptr<CachedPartition>(
        new CachedPartition(identity->dataset_path(), identity->file_path()));
      RETURN_IF_ERROR(dataset_cache_block_manager->InsertPartition(identity, new_partition));
    }

    // Insert the columns into dataset_cache_block_manager.
    for(auto iter = retrieved_columns.begin(); iter != retrieved_columns.end(); iter ++) {
      RETURN_IF_ERROR(dataset_cache_block_manager->InsertColumn(identity, iter->first, iter->second));
    }
}
// method name action
Status RetrieveAndCacheAndInsertColumns(Identity* identity, std::shared_ptr<StoragePluginFactory> storage_plugin_factory,
 std::vector<int> col_ids, std::shared_ptr<DatasetCacheBlockManager> dataset_cache_block_manager,
  std::shared_ptr<DatasetCacheEngineManager> dataset_cache_engine_manager) {
    std::string partition_path = identity->file_path();
    std::shared_ptr<StoragePlugin> storage_plugin;

    // Get the ReadableFile Debug Check
    RETURN_IF_ERROR(storage_plugin_factory->GetStoragePlugin(partition_path, &storage_plugin));
    std::shared_ptr<HdfsReadableFile> file;
    RETURN_IF_ERROR(storage_plugin->GetReadableFile(partition_path, &file));

     // Get cache engine.
    std::shared_ptr<CacheEngine> cache_engine;
    CacheEngine::CachePolicy cache_policy = GetCachePolicy(identity);
    RETURN_IF_ERROR(dataset_cache_engine_manager->GetCacheEngine(cache_policy, &cache_engine));

    // Read the columns into ChunkArray.
    arrow::MemoryPool* memory_pool = new CacheMemoryPool(cache_engine);
    parquet::ArrowReaderProperties properties(new parquet::ArrowReaderProperties());
    std::unique_ptr<ParquetReader> parquet_reader(new ParquetReader(file, memory_pool, properties));
    std::unordered_map<string, std::shared_ptr<CachedColumn>> retrieved_columns;
  
    int64_t occupied_size = 0;
    for(auto iter = col_ids.begin(); iter != col_ids.end(); iter ++) {
      std::shared_ptr<arrow::ChunkedArray> chunked_out;
      RETURN_IF_ERROR(parquet_reader->ReadColumnChunk(*iter, chunked_out));
      arrow::ChunkedArray* chunked_array = chunked_out.get();
      int64_t column_size = memory_pool->bytes_allocated() - occupied_size;
      occupied_size = memory_pool->bytes_allocated() + occupied_size;
      std::shared_ptr<CacheRegion> cache_region = std::shared_ptr<CacheRegion>(new CacheRegion(0, column_size, column_size, chunked_array));
      std::shared_ptr<CachedColumn> column = std::shared_ptr<CachedColumn>(
        new CachedColumn(partition_path, *iter, cache_region));
      retrieved_columns.insert(std::make_pair(std::to_string(*iter), column));
      RETURN_IF_ERROR(cache_engine->PutValue(partition_path, *iter, cache_region));
    }
    InsertColumnsToBlockManager(identity, dataset_cache_block_manager, retrieved_columns);
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
  RETURN_IF_ERROR(dataset_cache_block_manager_->GetCachedColumns(identity, &cached_columns));

  std::shared_ptr<Table> table;
  for(auto iter = cached_columns.begin(); iter != cached_columns.end(); iter ++) {
    std::shared_ptr<CachedColumn> cache_column = iter->second;
    std::shared_ptr<CacheRegion> cache_region = cache_column->cache_region_;
    std::shared_ptr<arrow::ChunkedArray> chunked_out(cache_region->chunked_array());
    RETURN_IF_ERROR(Status::fromArrowStatus(Table::FromChunkedStructArray(chunked_out, &table)));
  }
  *data_stream = std::unique_ptr<rpc::FlightDataStream>(
    new rpc::RecordBatchStream(std::shared_ptr<RecordBatchReader>(new TableBatchReader(*table))));
}

Status RetrieveAndCacheAndInsertAndWrapStream(Identity* identity, std::shared_ptr<StoragePluginFactory> storage_plugin_factory,
 std::vector<int> col_ids, std::shared_ptr<DatasetCacheBlockManager> dataset_cache_block_manager,
  std::unique_ptr<rpc::FlightDataStream>* data_stream, std::shared_ptr<DatasetCacheEngineManager> dataset_cache_engine_manager) {
    RETURN_IF_ERROR(RetrieveAndCacheAndInsertColumns(identity, storage_plugin_factory, col_ids, dataset_cache_block_manager, dataset_cache_engine_manager));
    RETURN_IF_ERROR(WrapDatasetStream(data_stream, dataset_cache_block_manager, identity));
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
    RetrieveAndCacheAndInsertAndWrapStream(identity, storage_plugin_factory_, col_ids,
     dataset_cache_block_manager_, data_stream, dataset_cache_engine_manager_);
  } else {
    // dataset is cached
    std::shared_ptr<CachedPartition> partition;
    dataset_cache_block_manager_->GetCachedPartition(identity, &partition);
    if (partition == NULL) {
      LOG(WARNING) << "The partition "<< identity->file_path() 
      <<" is NULL. We will get all the columns from storage and then insert the column into dataset cache block manager";
      RetrieveAndCacheAndInsertAndWrapStream(identity, storage_plugin_factory_, col_ids,
       dataset_cache_block_manager_, data_stream, dataset_cache_engine_manager_);
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
        RetrieveAndCacheAndInsertAndWrapStream(identity, storage_plugin_factory_, uncached_col_ids,
         dataset_cache_block_manager_, data_stream, dataset_cache_engine_manager_);
      }
   }
  }
}

} // namespace pegasus