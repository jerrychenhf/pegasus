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

#ifndef PEGASUS_CACHE_ENGINE_H
#define PEGASUS_CACHE_ENGINE_H

#include "common/status.h"
#include "dataset/dataset.h"
#include <boost/compute/detail/lru_cache.hpp>
#include "cache/cache_region.h"
#include "boost/functional/hash.hpp"
#include "dataset/cache_store_manager.h"
#include "cache/lru_cache.h"

using namespace boost;

namespace pegasus {

class CacheEngine {
 public:
 virtual Status Init() = 0;
 virtual Status GetCacheStore(CacheStore** cache_store) = 0;
 virtual Status PutValue(LRUCache::CacheKey key) = 0;

  enum CachePolicy {
    LRU,
    NonEvict,
  };

 private:
  CachePolicy cache_policy;
};

class LruCacheEngine : public CacheEngine {
 public:
  LruCacheEngine(int64_t capacity);
  ~LruCacheEngine() {
    if (lru_cache_ != nullptr) {
      delete lru_cache_;
      lru_cache_ = nullptr;
    }
  }
  
  virtual Status Init();
  
  Status GetCacheStore(CacheStore** cache_store) override {
    return cache_store_manager_->GetCacheStore(cache_store);
  }

  Status PutValue(LRUCache::CacheKey key) override;

 public:
  std::shared_ptr<CacheStoreManager> cache_store_manager_;
  LRUCache* lru_cache_;
};

//NonEvictCacheEngine 
class NonEvictionCacheEngine : public CacheEngine {
 public:
  NonEvictionCacheEngine() {};
  ~NonEvictionCacheEngine() {};

  virtual Status Init() {
    return Status::OK();
  }
  
  Status GetCacheStore(CacheStore** cache_store) override {
    return Status::NotImplemented("Not yet implemented.");
  }
  Status PutValue(LRUCache::CacheKey key) override {
    return Status::NotImplemented("Not yet implemented.");
  }
};
} // namespace pegasus                              

#endif  // PEGASUS_CACHE_ENGINE_H