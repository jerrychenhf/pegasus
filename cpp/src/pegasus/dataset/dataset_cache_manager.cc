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
#include <boost/thread/mutex.hpp>

namespace pegasus {

DatasetCacheManager::DatasetCacheManager()
  : cache_engine_manager_(nullptr),
    cache_block_manager_(nullptr) {
}

DatasetCacheManager::~DatasetCacheManager() {
  if (cache_engine_manager_ != nullptr) {
    delete cache_engine_manager_;
    cache_engine_manager_ = nullptr;
  }

  if (cache_block_manager_ != nullptr) {
    delete cache_block_manager_;
    cache_block_manager_ = nullptr;
  }
}

Status DatasetCacheManager::Init() {
  ExecEnv* env =  ExecEnv::GetInstance();
  storage_plugin_factory_ = env->get_storage_plugin_factory();
   
  cache_block_manager_ = new DatasetCacheBlockManager();
  cache_engine_manager_ = new DatasetCacheEngineManager();
   
  RETURN_IF_ERROR(cache_block_manager_->Init());
  RETURN_IF_ERROR(cache_engine_manager_->Init());
   
  return Status::OK();
}

CacheEngine::CachePolicy DatasetCacheManager::GetCachePolicy(RequestIdentity* request_identity) {
  // TODO Choose the CachePolicy based on the data type in Identity
  return CacheEngine::CachePolicy::LRU;
}

Status DatasetCacheManager::GetPartition(RequestIdentity* request_identity,
 std::shared_ptr<CachedPartition>* new_partition) {
    
    std::shared_ptr<CachedDataset> dataset;
    RETURN_IF_ERROR(cache_block_manager_->GetCachedDataSet(
      request_identity->dataset_path(), &dataset));
    
    RETURN_IF_ERROR(dataset->GetCachedPartition(dataset,
     request_identity->partition_path(), new_partition));
    return Status::OK();
}

Status DatasetCacheManager::WrapDatasetStream(RequestIdentity* request_identity,
 unordered_map<int, std::shared_ptr<CachedColumn>> request_columns,
  std::unique_ptr<rpc::FlightDataStream>* data_stream) {
  LOG(WARNING) << "Wrap the dataset into flight data stream";

  // std::shared_ptr<Table> table;
  std::vector<std::shared_ptr<ChunkedArray>> chunked_arrays;
  std::vector<std::shared_ptr<CachedColumn>> columns;

  std::vector<int> col_ids = request_identity->column_indices();
  for(int index : col_ids) {
    auto iter = request_columns.find(index);
    if (iter != request_columns.end()) {
      std::shared_ptr<CachedColumn> cache_column = iter->second;
      CacheRegion* cache_region = cache_column->GetCacheRegion();
      std::shared_ptr<ChunkedArray> chunked_array = cache_region->chunked_array();
      chunked_arrays.push_back(chunked_array);
      columns.push_back(cache_column);
    } else {
      return Status::ObjectNotFound("can't find the cached column.");
    }
  }

  std::shared_ptr<arrow::Schema> schema;
  request_identity->get_schema(&schema);

  std::shared_ptr<arrow::Table> table = arrow::Table::Make(schema, chunked_arrays);

  // *data_stream = std::unique_ptr<rpc::FlightDataStream>(
  //   new rpc::RecordBatchStream(std::shared_ptr<RecordBatchReader>(
  //     new TableBatchReader(*table))));
  *data_stream = std::unique_ptr<rpc::FlightDataStream>(
  new rpc::TableRecordBatchStream(table, columns));

  return Status::OK();
}

Status DatasetCacheManager::GetDatasetStreamWithMissedColumns(RequestIdentity* request_identity,
  std::vector<int> col_ids,
  unordered_map<int, std::shared_ptr<CachedColumn>> cached_columns,
  std::unique_ptr<rpc::FlightDataStream>* data_stream) {
     // Get cache engine.
    std::shared_ptr<CacheEngine> cache_engine;
    CacheEngine::CachePolicy cache_policy = GetCachePolicy(request_identity);
    RETURN_IF_ERROR(cache_engine_manager_->GetCacheEngine(cache_policy, &cache_engine));

    unordered_map<int, std::shared_ptr<CachedColumn>> retrieved_columns;
    RETURN_IF_ERROR(RetrieveColumns(request_identity, col_ids, cache_engine, retrieved_columns));

    for (auto iter = cached_columns.begin(); iter != cached_columns.end(); iter ++) {
      int col_id = iter->first;
      std::shared_ptr<CachedColumn> cached_column = iter->second;
      retrieved_columns.insert(std::make_pair(col_id, cached_column));
    }
    
    return WrapDatasetStream(request_identity, retrieved_columns, data_stream);
}

Status DatasetCacheManager::RetrieveColumns(RequestIdentity* request_identity,
  const std::vector<int>& col_ids,
  std::shared_ptr<CacheEngine> cache_engine,
  unordered_map<int, std::shared_ptr<CachedColumn>>& retrieved_columns) {
    LOG(WARNING) << "Retrieve the columns from storage and insert the"
     << "retrieved columns into dataset block manager and cache engine";
    std::string dataset_path = request_identity->dataset_path();
    std::string partition_path = request_identity->partition_path();

    // Get the ReadableFile
    std::shared_ptr<StoragePlugin> storage_plugin;
    RETURN_IF_ERROR(storage_plugin_factory_->GetStoragePlugin(partition_path, &storage_plugin));
    std::shared_ptr<HdfsReadableFile> file;
    RETURN_IF_ERROR(storage_plugin->GetReadableFile(partition_path, &file));
    
    // Read the columns into ChunkArray.
    // Asumming the cache memory pool is only in same store.
    std::shared_ptr<CacheMemoryPool> memory_pool(new CacheMemoryPool(cache_engine));
    RETURN_IF_ERROR(memory_pool->Create());
    
    parquet::ArrowReaderProperties properties(parquet::default_arrow_reader_properties());
    std::unique_ptr<ParquetReader> parquet_reader(new ParquetReader(file, memory_pool.get(), properties));

    std::shared_ptr<CachedPartition> partition;
    GetPartition(request_identity, &partition);
    
    int64_t occupied_size = 0;
    for(auto iter = col_ids.begin(); iter != col_ids.end(); iter ++) {
      int colId = *iter;
      std::shared_ptr<arrow::ChunkedArray> chunked_array;
      RETURN_IF_ERROR(parquet_reader->ReadColumnChunk(*iter, &chunked_array));
      int64_t column_size = memory_pool->bytes_allocated() - occupied_size;
      occupied_size += column_size;
      CacheRegion* cache_region = new CacheRegion(memory_pool,
        chunked_array, column_size);
      std::shared_ptr<CachedColumn> column = std::shared_ptr<CachedColumn>(
        new CachedColumn(partition_path, colId, cache_region));

      // Insert the column if already inserted, 
      // we do not put the column into the LRU cache.
      // And because this column is shared ptr, so out this for clause , 
      // it will be delete automatically.
      std::shared_ptr<CachedColumn> cached_column;
      bool is_inserted = partition->InsertColumn(partition, colId, column, &cached_column);
      if (is_inserted && cached_column == nullptr) {
        LRUCache::CacheKey key(dataset_path, partition_path, colId, column_size);
        LOG(WARNING) << "Put the cached column into cache engine";
        RETURN_IF_ERROR(cache_engine->PutValue(key));

        retrieved_columns.insert(std::make_pair(colId, column));
      } else {
        retrieved_columns.insert(std::make_pair(colId, cached_column));
      }
    }
    
    return Status::OK();
}

std::vector<int> DatasetCacheManager::GetMissedColumnsIds(std::vector<int> col_ids,
  unordered_map<int, std::shared_ptr<CachedColumn>> cached_columns) {
   LOG(WARNING) << "Get the missed column IDs ";
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
  unordered_map<int, std::shared_ptr<CachedColumn>> cached_columns;

  std::shared_ptr<CachedDataset> dataset;
  cache_block_manager_->GetCachedDataSet(request_identity->dataset_path(), &dataset);
  if (dataset->GetCachedPartitions().size() == 0) {
    LOG(WARNING) << "The dataset "<< request_identity->dataset_path() 
    <<" is new added. We will get all the columns from storage and"
     << " then insert the column into dataset cache block manager";
    return GetDatasetStreamWithMissedColumns(request_identity, col_ids, cached_columns, data_stream);
  } else {
    // dataset is cached
    std::shared_ptr<CachedPartition> partition;
    dataset->GetCachedPartition(dataset,
     request_identity->partition_path(), &partition);
    if (partition->GetCachedColumns().size() == 0) {
      LOG(WARNING) << "The partition "<< request_identity->partition_path() 
      <<" is new added. We will get all the columns from storage and"
       << "then insert the column into dataset cache block manager";
      return GetDatasetStreamWithMissedColumns(request_identity, col_ids, cached_columns, data_stream);
    } else {
      // partition is cached.
      // Check which column is cached.
      partition->GetCachedColumns(partition, request_identity->column_indices(), &cached_columns);
      if (col_ids.size() == cached_columns.size()) {
        LOG(WARNING) << "All the columns are cached. And we will wrap the columns into Flight data stream";
        return WrapDatasetStream(request_identity, cached_columns, data_stream);
      } else {
        // Not all columns cached.
        // Get the not cached col_ids.
        std::vector<int> missed_col_ids = GetMissedColumnsIds(col_ids, cached_columns);
        LOG(WARNING) << "Partial columns is cached and we will get the missed columns from storage";
        return GetDatasetStreamWithMissedColumns(request_identity, missed_col_ids, cached_columns, data_stream);
      }
   }
  }
}

} // namespace pegasus