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
#include "runtime/worker_exec_env.h"
#include "parquet/parquet_reader.h"
#include "cache/cache_memory_pool.h"
#include "common/logging.h"
#include "cache/lru_cache.h"

namespace pegasus {

DatasetCacheManager::DatasetCacheManager() {
   ExecEnv* env =  ExecEnv::GetInstance();
   storage_plugin_factory_ = env->get_storage_plugin_factory();
}

DatasetCacheManager::~DatasetCacheManager() {
}

Status DatasetCacheManager::Init() {
  cache_block_manager_ = std::shared_ptr<DatasetCacheBlockManager>(
     new DatasetCacheBlockManager());
  cache_engine_manager_ = std::shared_ptr<DatasetCacheEngineManager>
   (new DatasetCacheEngineManager());
   
  RETURN_IF_ERROR(cache_block_manager_->Init());
  RETURN_IF_ERROR(cache_engine_manager_->Init());
   
  return Status::OK();
}

CacheEngine::CachePolicy DatasetCacheManager::GetCachePolicy(RequestIdentity* request_identity) {
  // TODO Choose the CachePolicy based on the data type in Identity
  return CacheEngine::CachePolicy::LRU;
}

Status DatasetCacheManager::AddNewColumns(RequestIdentity* request_identity,
  unordered_map<int, std::shared_ptr<CachedColumn>> retrieved_columns) {
    // Before insert into the column, check whether the dataset is inserted.
    std::shared_ptr<CachedDataset> dataset;
    RETURN_IF_ERROR(cache_block_manager_->GetCachedDataSet(request_identity->dataset_path(), &dataset));
    if (dataset == nullptr) {
      // Insert new dataset.
      std::shared_ptr<CachedDataset> new_dataset = std::shared_ptr<CachedDataset>(
        new CachedDataset(request_identity->dataset_path()));
      RETURN_IF_ERROR(cache_block_manager_->InsertDataSet(request_identity->dataset_path(), new_dataset));
    }
    // After check the dataset, continue to check whether the partition is inserted.
    std::shared_ptr<CachedPartition> partition;
    RETURN_IF_ERROR(cache_block_manager_->GetCachedPartition(request_identity->dataset_path(),
     request_identity->partition_path(), &partition));
    if (partition == nullptr) {
      std::shared_ptr<CachedPartition> new_partition = std::shared_ptr<CachedPartition>(
        new CachedPartition(request_identity->dataset_path(), request_identity->partition_path()));
      RETURN_IF_ERROR(cache_block_manager_->InsertPartition(request_identity->dataset_path(),
     request_identity->partition_path(), new_partition));
    }

    // Insert the columns into cache_block_manager_.
    for(auto iter = retrieved_columns.begin(); iter != retrieved_columns.end(); iter ++) {
      printf("inserting new columns\n");
    
      RETURN_IF_ERROR(cache_block_manager_->InsertColumn(request_identity->dataset_path(),
        request_identity->partition_path(), iter->first, iter->second));
    }
    return Status::OK();
}

Status DatasetCacheManager::WrapDatasetStream(RequestIdentity* request_identity,
  std::unique_ptr<rpc::FlightDataStream>* data_stream) {
  unordered_map<int, std::shared_ptr<CachedColumn>> cached_columns;
  RETURN_IF_ERROR(cache_block_manager_->GetCachedColumns(request_identity->dataset_path(),
     request_identity->partition_path(), request_identity->column_indices(), &cached_columns));

  std::shared_ptr<Table> table;
  printf("the size is %d \n", cached_columns.size());
  for(auto iter = cached_columns.begin(); iter != cached_columns.end(); iter ++) {
    std::shared_ptr<CachedColumn> cache_column = iter->second;
    CacheRegion* cache_region = cache_column->cache_region_;
    
    RETURN_IF_ERROR(Status::fromArrowStatus(Table::FromChunkedStructArray(
      cache_region->chunked_array(), &table)));
  }
  *data_stream = std::unique_ptr<rpc::FlightDataStream>(
    new rpc::RecordBatchStream(std::shared_ptr<RecordBatchReader>(new TableBatchReader(*table))));
  return Status::OK();
}

Status DatasetCacheManager::GetDatasetStreamWithMissedColumns(RequestIdentity* request_identity,
  std::vector<int> col_ids,
  std::unique_ptr<rpc::FlightDataStream>* data_stream) {
     // Get cache engine.
    std::shared_ptr<CacheEngine> cache_engine;
    CacheEngine::CachePolicy cache_policy = GetCachePolicy(request_identity);
    RETURN_IF_ERROR(cache_engine_manager_->GetCacheEngine(cache_policy, &cache_engine));

    unordered_map<int, std::shared_ptr<CachedColumn>> retrieved_columns;
    RETURN_IF_ERROR(RetrieveColumns(request_identity, col_ids, cache_engine, retrieved_columns));
    
    RETURN_IF_ERROR(AddNewColumns(request_identity, retrieved_columns));
    return WrapDatasetStream(request_identity, data_stream);
}

Status DatasetCacheManager::RetrieveColumns(RequestIdentity* request_identity,
  const std::vector<int>& col_ids,
  std::shared_ptr<CacheEngine> cache_engine,
  unordered_map<int, std::shared_ptr<CachedColumn>>& retrieved_columns) {
    std::string dataset_path = request_identity->dataset_path();
    std::string partition_path = request_identity->partition_path();
    std::shared_ptr<StoragePlugin> storage_plugin;

    // Get the ReadableFile
    RETURN_IF_ERROR(storage_plugin_factory_->GetStoragePlugin(partition_path, &storage_plugin));
    std::shared_ptr<HdfsReadableFile> file;
    RETURN_IF_ERROR(storage_plugin->GetReadableFile(partition_path, &file));
    
    // Read the columns into ChunkArray.
    // Asumming the cache memory pool is only in same store.
    std::shared_ptr<CacheMemoryPool> memory_pool(new CacheMemoryPool(cache_engine));
    RETURN_IF_ERROR(memory_pool->Create());
    
    CacheStore* cache_store = memory_pool->GetCacheStore();
    parquet::ArrowReaderProperties properties(parquet::default_arrow_reader_properties());
    std::unique_ptr<ParquetReader> parquet_reader(new ParquetReader(file, memory_pool.get(), properties));
    
    int64_t occupied_size = 0;
    for(auto iter = col_ids.begin(); iter != col_ids.end(); iter ++) {
      int colId = *iter;
      std::shared_ptr<arrow::ChunkedArray> chunked_array;
      RETURN_IF_ERROR(parquet_reader->ReadColumnChunk(*iter, &chunked_array));
      int64_t column_size = memory_pool->bytes_allocated() - occupied_size;
      occupied_size = memory_pool->bytes_allocated() + occupied_size;
      CacheRegion* cache_region = new CacheRegion(memory_pool,
        chunked_array, column_size);
         printf("the cache region size when retrieving is %d \n", column_size);
      std::shared_ptr<CachedColumn> column = std::shared_ptr<CachedColumn>(
        new CachedColumn(partition_path, colId, cache_region));
      retrieved_columns.insert(std::make_pair(*iter, column));
      
      LRUCache::CacheKey key(dataset_path, partition_path, colId, column_size, cache_block_manager_);
      RETURN_IF_ERROR(cache_engine->PutValue(key));
    }
    
    return Status::OK();
}

std::vector<int> DatasetCacheManager::GetMissedColumnsIds(std::vector<int> col_ids,
  unordered_map<int, std::shared_ptr<CachedColumn>> cached_columns) {
   std::vector<int> missed_col_ids;
    for(auto iter = col_ids.begin(); iter != col_ids.end(); iter ++) {
        auto entry = cached_columns.find(*iter);
        if (entry == cached_columns.end()) {
            missed_col_ids.push_back(*iter);
        }
    }
    return missed_col_ids;
}

// Wrap the data to flight data stream.
// According DatasetCacheBlockManager to chech whether sotred.
// If yes, read it based on the address stored in DatasetCacheBlockManager and then return.
// If not, get dataset from hdfs and then put the dataset into CacheEngine.
//         1. Choose the CachePolicy based on the Identity.
//         2. Call DatasetCacheEngineManager#GetCacheEngine method to get CacheEngine;
Status DatasetCacheManager::GetDatasetStream(RequestIdentity* request_identity,
 std::unique_ptr<rpc::FlightDataStream>* data_stream) {
  // Check whether the dataset is cached.
  
  std::vector<int> col_ids = request_identity->column_indices();
  std::unordered_map<string, std::shared_ptr<CachedColumn>> get_columns;
  
  std::shared_ptr<CachedDataset> dataset;
  cache_block_manager_->GetCachedDataSet(request_identity->dataset_path(), &dataset);
  if (dataset == nullptr) {
    LOG(WARNING) << "The dataset "<< request_identity->dataset_path() 
    <<" is nullptr. We will get all the columns from storage and"
     << "then insert the column into dataset cache block manager";
    return GetDatasetStreamWithMissedColumns(request_identity, col_ids, data_stream);
  } else {
    // dataset is cached
    std::shared_ptr<CachedPartition> partition;
    cache_block_manager_->GetCachedPartition(request_identity->dataset_path(),
     request_identity->partition_path(), &partition);
    if (partition == nullptr) {
      LOG(WARNING) << "The partition "<< request_identity->partition_path() 
      <<" is nullptr. We will get all the columns from storage and"
       << "then insert the column into dataset cache block manager";
      return GetDatasetStreamWithMissedColumns(request_identity, col_ids, data_stream);
    } else {
      // partition is cached.
      // Check which column is cached.
      unordered_map<int, std::shared_ptr<CachedColumn>> cached_columns;
      cache_block_manager_->GetCachedColumns(request_identity->dataset_path(),
       request_identity->partition_path(), request_identity->column_indices(), &cached_columns);
      if (col_ids.size() == cached_columns.size()) {
        LOG(WARNING) << "All the columns are cached. And we will wrap the columns into Flight data stream";
        return WrapDatasetStream(request_identity, data_stream);
      } else {
        // Not all columns cached.
        // Get the not cached col_ids.
        std::vector<int> missed_col_ids = GetMissedColumnsIds(col_ids, cached_columns);
        return GetDatasetStreamWithMissedColumns(request_identity, missed_col_ids, data_stream);
      }
   }
  }
}

} // namespace pegasus