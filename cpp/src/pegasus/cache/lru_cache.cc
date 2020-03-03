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

#include "cache/lru_cache.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <ostream>
#include <string>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "gutil/gscoped_ptr.h"
#include "gutil/macros.h"
#include "gutil/strings/substitute.h"
#include "util/cache.h"
#include "util/flag_tags.h"
#include "util/flag_validators.h"
#include "util/slice.h"
#include "util/string_case.h"
#include "common/logging.h"

#include "runtime/worker_exec_env.h"
#include "dataset/dataset_cache_manager.h"

DEFINE_int64(lru_cache_capacity_mb, 512, "lru cache capacity in MB");
TAG_FLAG(lru_cache_capacity_mb, stable);

// Yes, it's strange: the default value is 'true' but that's intentional.
// The idea is to avoid the corresponding group flag validator striking
// while running anything but master and tserver. As for the master and tserver,
// those have the default value for this flag set to 'false'.
DEFINE_bool(force_lru_cache_capacity, true,
            "Force pegasus to accept the lru cache size, even if it is unsafe.");
TAG_FLAG(force_lru_cache_capacity, unsafe);
TAG_FLAG(force_lru_cache_capacity, hidden);

DEFINE_string(lru_cache_type, "DRAM",
              "Which type of lru cache to use for caching data. "
              "Valid choices are 'DRAM' or 'NVM'. DRAM, the default, "
              "caches data in regular memory. 'NVM' caches data "
              "in a memory-mapped file using the memkind library. To use 'NVM', "
              "libmemkind 1.8.0 or newer must be available on the system; "
              "otherwise pegasus will crash.");

using strings::Substitute;
using std::string;

template <class T> class scoped_refptr;

namespace pegasus {

class MetricEntity;

Cache* CreateCache(int64_t capacity) {
  const auto mem_type = LRUCache::GetConfiguredCacheMemoryTypeOrDie();
  switch (mem_type) {
    case Cache::MemoryType::DRAM:
      return NewCache<Cache::EvictionPolicy::LRU, Cache::MemoryType::DRAM>(
          capacity, "lru_cache");
    // case Cache::MemoryType::NVM:
    //   return NewCache<Cache::EvictionPolicy::LRU, Cache::MemoryType::NVM>(
    //       capacity, "lru_cache");
    default:
      LOG(FATAL) << "unsupported LRU cache memory type: " << mem_type;
      return nullptr;
  }
}

bool ValidateLRUCacheCapacity() {
  if (FLAGS_force_lru_cache_capacity) {
    return true;
  }
  if (FLAGS_lru_cache_type != "DRAM") {
    return true;
  }
  int64_t capacity = FLAGS_lru_cache_capacity_mb * 1024 * 1024;
//   int64_t mpt = process_memory::MemoryPressureThreshold();
//   if (capacity > mpt) {
//     LOG(ERROR) << Substitute("lru cache capacity exceeds the memory pressure "
//                              "threshold ($0 bytes vs. $1 bytes). This will "
//                              "cause instability and harmful flushing behavior. "
//                              "Lower --lru_cache_capacity_mb or raise "
//                              "--memory_limit_hard_bytes.",
//                              capacity, mpt);
//     return false;
//   }
//   if (capacity > mpt / 2) {
//     LOG(WARNING) << Substitute("lru cache capacity exceeds 50% of the memory "
//                                "pressure threshold ($0 bytes vs. 50% of $1 bytes). "
//                                "This may cause performance problems. Consider "
//                                "lowering --lru_cache_capacity_mb or raising "
//                                "--memory_limit_hard_bytes.",
//                                capacity, mpt);
//   }
  return true;
}


// GROUP_FLAG_VALIDATOR(lru_cache_capacity_mb, ValidateLRUCacheCapacity);

Cache::MemoryType LRUCache::GetConfiguredCacheMemoryTypeOrDie() {
    // ToUpperCase(FLAGS_lru_cache_type, &FLAGS_lru_cache_type);
  if (FLAGS_lru_cache_type == "NVM") {
    return Cache::MemoryType::NVM;
  }
  if (FLAGS_lru_cache_type == "DRAM") {
    return Cache::MemoryType::DRAM;
  }

  LOG(FATAL) << "Unknown lru cache type: '" << FLAGS_lru_cache_type
             << "' (expected 'DRAM' or 'NVM')";
  __builtin_unreachable();
}

LRUCache::LRUCache()
    : LRUCache(FLAGS_lru_cache_capacity_mb * 1024 * 1024) {
}

LRUCache::LRUCache(size_t capacity)
    : cache_(CreateCache(capacity)) {
}

Status LRUCache::Init() {
  WorkerExecEnv* env = WorkerExecEnv::GetInstance();
  dataset_cache_block_manager_ = env->GetDatasetCacheManager()->cache_block_manager_;
  eviction_callback_ = new LRUEvictionCallback(this);
}

LRUCache::~LRUCache() {
  // the eviction call back may be not need to delete??
  // if (eviction_callback_ != nullptr) {
  //   delete eviction_callback_;
  //   eviction_callback_ = nullptr;
  // }
  // here the cache block manager will be delete in cache manager.
  // if (dataset_cache_block_manager_ != nullptr) {
  //   delete dataset_cache_block_manager_;
  //   dataset_cache_block_manager_ = nullptr;
  // }
}

LRUCache::PendingEntry LRUCache::Allocate(const CacheKey& key, size_t lru_size) {
  Slice key_slice(reinterpret_cast<const uint8_t*>(&key), sizeof(key));
  return PendingEntry(cache_->Allocate(key_slice, lru_size, key.occupied_size_));
}

bool LRUCache::Lookup(const CacheKey& key, Cache::CacheBehavior behavior,
                        LRUCacheHandle* handle) {
  auto h(cache_->Lookup(
      Slice(reinterpret_cast<const uint8_t*>(&key), sizeof(key)), behavior));
  if (h) {
    handle->SetHandle(std::move(h));
    return true;
  }
  return false;
}

void LRUCache::Insert(LRUCache::PendingEntry* entry, LRUCacheHandle* inserted) {
  auto h(cache_->Insert(std::move(entry->handle_),
                        eviction_callback_));
  inserted->SetHandle(std::move(h));
}

} // namespace pegasus
