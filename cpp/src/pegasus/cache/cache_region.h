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
#include "dataset/object_id.h"

#include <string>

using namespace std;

namespace arrow {
  class ChunkedArray;
  class MemoryPool;
}

namespace pegasus {
  
class CacheMemoryPool;

struct ObjectEntry {
  ObjectEntry();

  ~ObjectEntry();

  /// Memory mapped file containing the object.
  int fd;
  /// Offset from the base of the mmap.
  ptrdiff_t offset;
  /// Size of the object in bytes.
  int64_t data_size;
};

class CacheRegion {
 public:
  CacheRegion();
  CacheRegion(const std::shared_ptr<CacheMemoryPool>& memory_pool,
    const std::shared_ptr<arrow::ChunkedArray>& chunked_array, int64_t size,
     const unordered_map<int, std::shared_ptr<ObjectID>> object_ids = {});
  
  ~CacheRegion();
  
  int64_t size() const;
  std::shared_ptr<arrow::ChunkedArray> chunked_array() const;
  std::shared_ptr<CacheMemoryPool> memory_pool() const;
  unordered_map<int, std::shared_ptr<ObjectID>> object_ids() const;
 private:
  // the pool object associated to the chunked array
  // use shared ptr to managed the life time
  std::shared_ptr<CacheMemoryPool> memory_pool_;
  std::shared_ptr<arrow::ChunkedArray> chunked_array_;
  int64_t size_;
  // the key is the rowgroup_ids and the value is the objectid
  unordered_map<int, std::shared_ptr<ObjectID>> object_ids_;
};

} // namespace pegasus

#endif  // PEGASUS_CACHE_REGION_H
