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

#include "pegasus/common/status.h"
#include "pegasus/dataset/dataset.h"
#include <boost/compute/detail/lru_cache.hpp>
#include "pegasus/util/lru_cache.h"
#include "pegasus/cache/cache_entry_holder.h"

namespace pegasus {

class CacheEngine {
 public:

  enum CachePolicy {
    LRU,
    NonLRU,
  };

 private:
  CachePolicy cache_policy;
};

class CacheKey {
 public:
  explicit CacheKey(string file_path, int row_group_id, int column_id) : file_path_(file_path),
  row_group_id_(row_group_id), column_id_(column_id) {}
  
 private:
  string file_path_;
  int row_group_id_;
  int column_id_;
};

class LruCacheEngine : public CacheEngine {
 public:
  LruCacheEngine(long capacity);
  ~LruCacheEngine();

  Status PutValue();

 private:
  LruCache<CacheKey, CacheEntryHolder> cache_;
};

class NonLruCacheEngine : public CacheEngine {
 public:
  NonLruCacheEngine();
  ~NonLruCacheEngine();

  Status PutValue();

 private:
};
} // namespace pegasus                              

#endif  // PEGASUS_CACHE_ENGINE_H