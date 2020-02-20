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

#ifndef PEGASUS_MEMORY_BLOCK_HOLDER_H
#define PEGASUS_MEMORY_BLOCK_HOLDER_H

#include "dataset/cache_store.h"

#include <string>
#include "arrow/table.h"

using namespace std;
using namespace arrow;

namespace pegasus {

class CacheRegion {
 public:
  CacheRegion();
  CacheRegion(arrow::ChunkedArray* chunked_array, int64_t size);
  
  ~CacheRegion();
  int64_t size() const;
  arrow::ChunkedArray* chunked_array() const;
  

 private:
  int64_t size_;
  arrow::ChunkedArray* chunked_array_; 
};

} // namespace pegasus

#endif  // PEGASUS_MEMORY_BLOCK_HOLDER_H