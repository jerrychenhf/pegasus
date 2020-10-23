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

#ifndef PEGASUS_DCPMM_STORE_H
#define PEGASUS_DCPMM_STORE_H

#include <unordered_map>
#include <atomic>

#include "cache/store.h"
#include "common/status.h"
#include "cache/store_region.h"

struct memkind;

namespace pegasus {
class DCPMMStore : public Store {
 public:
  DCPMMStore(int64_t capacity);
  
  virtual Status Init(const std::unordered_map<string, string>* properties);
  
  virtual Status Allocate(int64_t size, StoreRegion* store_region) override;
  virtual Status Reallocate(int64_t old_size, int64_t new_size, StoreRegion* store_region) override;
  virtual Status Free(StoreRegion* store_region) override;
  
  virtual int64_t GetCapacity()override { return capacity_; }
  virtual int64_t GetFreeSize() override;
  virtual int64_t GetUsedSize() override;
  
  virtual std::string GetStoreName() override;
 private:
  int64_t capacity_;
  std::atomic<int64_t> used_size_;
  memkind* vmp_;
};

} // namespace pegasus

#endif  // PEGASUS_DCPMM_STORE_H