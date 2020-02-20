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
#include "cache/lru_cache.h"
#include "cache/cache_region.h"
#include "boost/functional/hash.hpp"
#include "dataset/cache_store_manager.h"

using namespace boost;

namespace pegasus {

class CacheEngine {
 public:
 virtual Status Init() = 0;
 virtual Status GetCacheStore(CacheStore** cache_store) = 0;
 virtual Status PutValue(std::string partition_path, int column_id,
   CacheRegion* cache_region, StoreRegion* store_region,
    CacheStore* cache_store) = 0;

  enum CachePolicy {
    LRU,
    NonEvict,
  };

 private:
  CachePolicy cache_policy;
};

class CacheEntryKey {
 public:
  explicit CacheEntryKey(std::string partition_path, int column_id)
   : partition_path_(partition_path), column_id_(column_id) {
    static const int kSeedValue = 4;
    size_t result = kSeedValue;

    boost::hash_combine(result, partition_path);
    boost::hash_combine(result, std::to_string(column_id));
    hash_code_ = result;
  }

  std::size_t Hash() const { return hash_code_; }

  bool operator==(const CacheEntryKey& other) const {
    // arrow schema does not overload equality operators.
    if (partition_path_ != other.partition_path_) {
      return false;
    }

    if (column_id_ != other.column_id_) {
      return false;
    }
    return true;
  }

  bool operator!=(const CacheEntryKey& other) const { return !(*this == other); }

  
 private:
  std::string partition_path_;
  int column_id_;
  size_t hash_code_;
};

class CacheEntryValue {
  public:
    CacheEntryValue(CacheRegion* cache_region, StoreRegion* store_region, CacheStore* cache_store):
     cache_region_(cache_region), store_region_(store_region), cache_store_(cache_store_) {}

    CacheRegion* cache_region() {
      return cache_region_;
    }

    StoreRegion* store_region() {
      return store_region_;
    }

    CacheStore* cache_store() {
      return cache_store_;
    }

  private:
    CacheRegion* cache_region_;
    StoreRegion* store_region_;
    CacheStore* cache_store_;
};

class LruCacheEngine : public CacheEngine {
 public:
  LruCacheEngine(int64_t capacity);
  ~LruCacheEngine() {};
  
  virtual Status Init();
  
  Status GetCacheStore(CacheStore** cache_store) override {
    return cache_store_manager_->GetCacheStore(cache_store);
  }

  Status PutValue(std::string partition_path, int column_id,
   CacheRegion* cache_region, StoreRegion* store_region,
    CacheStore* cache_store) override;

 public:
  std::shared_ptr<CacheStoreManager> cache_store_manager_;
  LruCache<CacheEntryKey, CacheEntryValue*> cache_;
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
  Status PutValue(std::string partition_path, int column_id,
   CacheRegion* cache_region, StoreRegion* store_region,
    CacheStore* cache_store) override {
    return Status::NotImplemented("Not yet implemented.");
  }
};
} // namespace pegasus                              

#endif  // PEGASUS_CACHE_ENGINE_H