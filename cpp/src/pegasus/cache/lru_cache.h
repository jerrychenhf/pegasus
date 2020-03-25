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
#ifndef PEGASUS_LRU_CACHE_H
#define PEGASUS_LRU_CACHE_H

#include <cstddef>
#include <cstdint>
#include <utility>

#include "gutil/gscoped_ptr.h"
#include "gutil/macros.h"
#include "gutil/port.h"
#include "gutil/singleton.h"
#include "util/cache.h"
#include "util/slice.h"

#include "dataset/dataset_cache_block_manager.h"

template <class T> class scoped_refptr;

namespace pegasus {

class MetricEntity;

class LRUCacheHandle;

// Wrapper around pegasus::Cache specifically for caching blocks of CFiles.
// Provides a singleton and LRU cache for CFile blocks.
class LRUCache {
 public:
  // Parse the gflag which configures the block cache. FATALs if the flag is
  // invalid.
  static Cache::MemoryType GetConfiguredCacheMemoryTypeOrDie();

  // The unique key identifying entries in the block cache.
  // Each cached block corresponds to a specific offset within
  // a file (called a "block" in other parts of pegasus).
  //
  // This structure's in-memory representation is internally memcpyed
  // and treated as a string. It may also be persisted across restarts
  // and upgrades of pegasus in persistent cache implementations. So, it's
  // important that the layout be fixed and kept compatible for all
  // future releases.
  struct CacheKey {
    CacheKey(std::string dataset_path, std::string partition_path,
     int column_id, int64_t occupied_size) :
      dataset_path_(dataset_path),
      partition_path_(partition_path),
      column_id_(column_id),
      occupied_size_(occupied_size){}

   std::string dataset_path_;
   std::string partition_path_;
   int column_id_;
   int64_t occupied_size_;
  };

  class LRUEvictionCallback : public Cache::EvictionCallback {
   public:
    explicit LRUEvictionCallback(
        DatasetCacheBlockManager* cache_block_manager):
         cache_block_manager_(cache_block_manager) {
           DCHECK(cache_block_manager_);
    }

    ~LRUEvictionCallback() {
      if(cache_block_manager_ != nullptr) {
        delete cache_block_manager_;
        cache_block_manager_ = nullptr;
      }
    }

    void EvictedEntry(Slice key, Slice val) override {
      // VLOG(2) << strings::Substitute("EvictedEntry callback for key '$0'",
      //                                key.ToString());
      auto* entry_ptr = reinterpret_cast<LRUCache::CacheKey*>(key.mutable_data());
  
      std::string dataset_path = entry_ptr->dataset_path_;
      std::string partition_path = entry_ptr->partition_path_;
      int column_id = entry_ptr->column_id_;
    
      if (cache_block_manager_ == nullptr ||
       cache_block_manager_->GetCachedDatasets().size() == 0) {
        return;
      }
      
       // Before insert into the column, check whether the dataset is inserted.
      std::shared_ptr<CachedDataset> dataset;
      cache_block_manager_->GetCachedDataSet(dataset_path, &dataset);
     

      if(dataset->GetCachedPartitions().size() == 0) {
        return;
      }
    
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
    DatasetCacheBlockManager* cache_block_manager_;
    
  };

  // An entry that is in the process of being inserted into the block
  // cache. See the documentation above 'Allocate' below on the block
  // cache insertion path.
  class PendingEntry {
   public:
    PendingEntry()
        : handle_(Cache::UniquePendingHandle(nullptr,
                                             Cache::PendingHandleDeleter(nullptr))) {
    }
    explicit PendingEntry(Cache::UniquePendingHandle handle)
        : handle_(std::move(handle)) {
    }
    PendingEntry(PendingEntry&& other) noexcept : PendingEntry() {
      *this = std::move(other);
    }

    ~PendingEntry() {
      reset();
    }

    PendingEntry& operator=(PendingEntry&& other) noexcept;
    PendingEntry& operator=(const PendingEntry& other) = delete;

    // Free the pending entry back to the block cache.
    // This is safe to call multiple times.
    void reset();

    // Return true if this is a valid pending entry.
    bool valid() const {
      return static_cast<bool>(handle_);
    }

    // Return the pointer into which the value should be written.
    uint8_t* val_ptr() {
      return handle_.get_deleter().cache()->MutableValue(&handle_);
    }

   private:
    friend class LRUCache;

    Cache::UniquePendingHandle handle_;
  };

  static LRUCache* GetSingleton() {
    return Singleton<LRUCache>::get();
  }

  explicit LRUCache(size_t capacity);

  // Pass a metric entity to the cache to start recording metrics.
  // This should be called before the block cache starts serving blocks.
  // Not calling StartInstrumentation will simply result in no block cache-related metrics.
  // Calling StartInstrumentation multiple times will reset the metrics each time.
  void StartInstrumentation(const scoped_refptr<MetricEntity>& metric_entity);

  // Insert the given block into the cache. 'inserted' is set to refer to the
  // entry in the cache.
  void Insert(const CacheKey* key);

  void Touch(const CacheKey* key);

  void Erase(const CacheKey& key);

  Status Init();

 private:
  friend class Singleton<LRUCache>;
  LRUCache();

  DISALLOW_COPY_AND_ASSIGN(LRUCache);

  gscoped_ptr<Cache> cache_;

  LRUEvictionCallback* eviction_callback_;
};

// Scoped reference to a block from the block cache.
class LRUCacheHandle {
 public:
  LRUCacheHandle()
      : handle_(Cache::UniqueHandle(nullptr, Cache::HandleDeleter(nullptr))) {
  }

  ~LRUCacheHandle() = default;

  // Swap this handle with another handle.
  // This can be useful to transfer ownership of a handle by swapping
  // with an empty LRUCacheHandle.
  void swap(LRUCacheHandle* dst) {
    std::swap(handle_, dst->handle_);
  }

  // Return the data in the cached block.
  //
  // NOTE: this slice is only valid until the block cache handle is
  // destructed or explicitly Released().
  Slice data() const {
    return handle_.get_deleter().cache()->Value(handle_);
  }

  bool valid() const {
    return static_cast<bool>(handle_);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(LRUCacheHandle);
  friend class LRUCache;

  void SetHandle(Cache::UniqueHandle handle) {
    handle_ = std::move(handle);
  }

  Cache::UniqueHandle handle_;
};


inline LRUCache::PendingEntry& LRUCache::PendingEntry::operator=(
    LRUCache::PendingEntry&& other) noexcept {
  reset();
  handle_ = std::move(other.handle_);
  return *this;
}

inline void LRUCache::PendingEntry::reset() {
  handle_.reset();
}

// Validates the block cache capacity. Won't permit the cache to grow large
// enough to cause pernicious flushing behavior. See pegasus-2318.
bool ValidateLRUCacheCapacity();

} // namespace pegasus

#endif // PEGASUS_LRU_CACHE_H
