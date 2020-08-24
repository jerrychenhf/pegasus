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
  class Buffer;
}

namespace pegasus {
  
class CacheMemoryPool;

struct ObjectEntry {
  ObjectEntry(int fd, ptrdiff_t offset, int64_t map_size, int row_counts):
  fd_(fd), offset_(offset), map_size_(map_size), row_counts_(row_counts) {};

  ~ObjectEntry() {};

  /// Memory mapped file containing the object.
  int fd_;
  /// Offset from the base of the mmap.
  ptrdiff_t offset_;
  /// Size of the underlying map.
  int64_t map_size_;

  int64_t row_counts_;
};

struct BufferEntry{
  BufferEntry(std::shared_ptr<arrow::Buffer> file_buffer, int64_t row_counts):
  file_buffer_(file_buffer), row_counts_(row_counts) {};

  ~BufferEntry() {};

  std::shared_ptr<arrow::Buffer> file_buffer_;
  int64_t row_counts_;

};

class CacheRegion {
 public:
  CacheRegion();
  CacheRegion(const std::shared_ptr<CacheMemoryPool>& memory_pool,
    const std::shared_ptr<arrow::ChunkedArray>& chunked_array, int64_t size,
     const unordered_map<int, std::shared_ptr<BufferEntry>> object_buffers = {},
    unordered_map<int, std::shared_ptr<ObjectEntry>> object_entries = {}, int64_t row_counts_per_rowgroup = 0 );
  
  ~CacheRegion();
  
  int64_t size() const;
  std::shared_ptr<arrow::ChunkedArray> chunked_array() const;
  std::shared_ptr<CacheMemoryPool> memory_pool() const;
  unordered_map<int, std::shared_ptr<BufferEntry>>& object_buffers();
  unordered_map<int, std::shared_ptr<ObjectEntry>>& object_entries();
  int64_t row_counts_per_rowgroup() const;
 private:
  // the pool object associated to the chunked array
  // use shared ptr to managed the life time
  std::shared_ptr<CacheMemoryPool> memory_pool_;
  std::shared_ptr<arrow::ChunkedArray> chunked_array_;
  int64_t size_;
  // the key is the rowgroup_ids and the value is the buffers
  // TODO: Combine the chunked_array_ and chunked_arrays_
  unordered_map<int, std::shared_ptr<BufferEntry>> object_buffers_;
  
  // store all the object entries for the column. the key is the row group ID.
  unordered_map<int, std::shared_ptr<ObjectEntry>> object_entries_;

  int64_t row_counts_per_rowgroup_;
  
};

} // namespace pegasus

#endif  // PEGASUS_CACHE_REGION_H
