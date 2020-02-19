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

#ifndef PEGASUS_CACHE_STORE_H
#define PEGASUS_CACHE_STORE_H

#include <vector>

#include "common/status.h"
#include "cache/store.h"
#include "cache/cache_region.h"

namespace pegasus {

class CacheStore {
  public:
   CacheStore(Store* store, int64_t capacity);
   ~CacheStore();
   
   Status Allocate(int64_t size, StoreRegion* store_region);
   Status Free(CacheRegion* cache_region);
   
   Store* GetStore() { return store_; }
   
   int64_t GetUsedSize() const { return used_size_; }
   int64_t GetCapacity() const { return capacity_; }
   
  private:
  Store* store_;
  int64_t capacity_;
  int64_t used_size_;
};
} // namespace pegasus                              

#endif  // PEGASUS_CACHE_STORE_H