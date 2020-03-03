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

#ifndef PEGASUS_LRU_EVICTION_CALLBACK_H
#define PEGASUS_LRU_EVICTION_CALLBACK_H

#include "util/cache.h"
#include "util/slice.h"

#include "dataset/dataset_cache_manager.h"
#include "dataset/dataset_cache_block_manager.h"
#include "runtime/worker_exec_env.h"

namespace pegasus {
class LRUEvictionCallback : public Cache::EvictionCallback {
   public:
    explicit LRUEvictionCallback() {
    }

    void EvictedEntry(Slice key, Slice val) override {
      // VLOG(2) << strings::Substitute("EvictedEntry callback for key '$0'",
      //                                key.ToString());
      auto* entry_ptr = reinterpret_cast<LRUCache::CacheKey*>(val.mutable_data());
      const std::string& dataset_path = entry_ptr->dataset_path_;
      const std::string& partition_path = entry_ptr->partition_path_;
      int column_id = entry_ptr->column_id_;

      WorkerExecEnv* worker_exec = WorkerExecEnv::GetInstance();
      std::shared_ptr<DatasetCacheManager> cache_manager = worker_exec->GetDatasetCacheManager();
      if (cache_manager->cache_block_manager_ == nullptr
       || cache_manager->cache_block_manager_->GetCachedDatasets().size() == 0) {
        return;
      }
       // Before insert into the column, check whether the dataset is inserted.
      std::shared_ptr<CachedDataset> dataset;
      cache_manager->cache_block_manager_->GetCachedDataSet(dataset_path, &dataset);
    
      // After check the dataset, continue to check whether the partition is inserted.
      std::shared_ptr<CachedPartition> partition;
      dataset->GetCachedPartition(dataset, partition_path, &partition);

      Status status = partition->DeleteColumn(
        partition, column_id);
      if (!status.ok()) {
        stringstream ss;
        ss << "Failed to delete the column when free the column";
        LOG(ERROR) << ss.str();
      }
    }

   private:
    DISALLOW_COPY_AND_ASSIGN(LRUEvictionCallback);
};
} // namespace pegasus

#endif // PEGASUS_LRU_EVICTION_CALLBACK_H