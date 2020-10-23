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

#ifndef PEGASUS_MEMORY_POOL_H
#define PEGASUS_MEMORY_POOL_H

#include <atomic>
#include <arrow/status.h>
#include <arrow/memory_pool.h>
#include "runtime/worker_exec_env.h"
#include "cache/cache_engine.h"
#include "cache/store.h"

using namespace arrow;

namespace pegasus
{

class MemoryPoolStats {
 public:
  MemoryPoolStats() : bytes_allocated_(0), max_memory_(0) {}

  int64_t max_memory() const { return max_memory_.load(); }
  int64_t bytes_allocated() const { return bytes_allocated_.load(); }

  inline void UpdateAllocatedBytes(int64_t diff) {
    auto allocated = bytes_allocated_.fetch_add(diff) + diff;
    // "maximum" allocated memory is ill-defined in multi-threaded code,
    // so don't try to be too rigorous here
    if (diff > 0 && allocated > max_memory_) {
      max_memory_ = allocated;
    }
  }
 protected:
  std::atomic<int64_t> bytes_allocated_;
  std::atomic<int64_t> max_memory_;
};

class CacheMemoryPool : public arrow::MemoryPool
{
public:
  CacheMemoryPool(std::shared_ptr<CacheEngine> cache_engine);
  ~CacheMemoryPool();
  
  Status Create();

  arrow::Status Allocate(int64_t size, uint8_t **out) override;
  arrow::Status Reallocate(int64_t old_size, int64_t new_size, uint8_t **ptr) override;
  void Free(uint8_t *buffer, int64_t size) override;
  
  int64_t bytes_allocated() const override;
  int64_t max_memory() const override;
  std::string backend_name() const override;

  CacheStore* GetCacheStore() { return cache_store_; }
  
  private:
    std::shared_ptr<CacheEngine> cache_engine_;
    CacheStore* cache_store_;
    MemoryPoolStats stats_;
    
    Status GetCacheRegion(int64_t size, StoreRegion* store_region);
};
} // namespace pegasus

#endif // PEGASUS_MEMORY_POOL_H