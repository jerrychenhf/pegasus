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
   CacheStore(long capacity, std::shared_ptr<Store> store);
   ~CacheStore();
   Status Allocate(long size, std::shared_ptr<CacheRegion>* cache_region);
   Status GetUsedSize(long& used_size);

  private:
  std::shared_ptr<Store> store_;
  long capacity_;
  long used_size_;
};
} // namespace pegasus                              

#endif  // PEGASUS_CACHE_STORE_H