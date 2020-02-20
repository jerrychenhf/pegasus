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

/// \brief Data structure providing an opaque identifier or credential to use
/// when requesting a data stream with the DoGet RPC
#include "cache/cache_region.h"
#include "cache/cache_memory_pool.h"

namespace pegasus {

CacheRegion::CacheRegion()
  : memory_pool_(nullptr), chunked_array_(nullptr), size_(0){
}

CacheRegion::CacheRegion(const std::shared_ptr<CacheMemoryPool>& memory_pool,
  const std::shared_ptr<arrow::ChunkedArray>& chunked_array, int64_t size)
  : memory_pool_(memory_pool), chunked_array_(chunked_array), size_(size) {
}

CacheRegion::~CacheRegion () {
}  

int64_t CacheRegion::size() const {
    return size_;
}

arrow::ChunkedArray* CacheRegion::chunked_array() const {
    return chunked_array_.get();
}

} // namespace pegasus
