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

#ifndef PEGASUS_STORE_H
#define PEGASUS_STORE_H

#include <unordered_map>
#include <atomic>
#include "common/status.h"
#include "cache/store_region.h"

struct memkind;

namespace pegasus {

class Store {
 public:
 
  virtual Status Init(const std::unordered_map<string, string>* properties) = 0;

  virtual Status Allocate(int64_t size, StoreRegion* store_region) = 0;
  virtual Status Reallocate(int64_t old_size, int64_t new_size, StoreRegion* store_region) = 0;
  virtual Status Free(StoreRegion* store_region) = 0;
  
  virtual int64_t GetCapacity() = 0;
  virtual int64_t GetFreeSize() = 0;
  virtual int64_t GetUsedSize() = 0;
  
  virtual std::string GetStoreName() = 0;

  enum StoreType {
    MEMORY,
    DCPMM,
    FILE
  };
};

} // namespace pegasus

#endif  // PEGASUS_STORE_H