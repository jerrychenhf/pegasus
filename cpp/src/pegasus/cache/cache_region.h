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

#ifndef PEGASUS_CACHE_REGION_H
#define PEGASUS_CACHE_REGION_H

#include "cache/cache_store.h"

#include <string>

using namespace std;

namespace arrow {
  class ChunkedArray;
  class MemoryPool;
}

namespace pegasus {
  
class CacheMemoryPool;

class CacheRegion {
 public:
  CacheRegion();
  CacheRegion(const std::shared_ptr<CacheMemoryPool>& memory_pool,
    const std::shared_ptr<arrow::ChunkedArray>& chunked_array, int64_t size);
  
  ~CacheRegion();
  
  int64_t size() const;
  std::shared_ptr<arrow::ChunkedArray> chunked_array() const;
  std::shared_ptr<CacheMemoryPool> memory_pool() const;
 private:
  // the pool object associated to the chunked array
  // use shared ptr to managed the life time
  std::shared_ptr<CacheMemoryPool> memory_pool_;
  std::shared_ptr<arrow::ChunkedArray> chunked_array_;
  int64_t size_;
};

} // namespace pegasus

#endif  // PEGASUS_CACHE_REGION_H
