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

#include "common/status.h"
#include "cache/store_region.h"
#include "cache/cache_region.h"

namespace pegasus {

class Store {
 public:
 
  virtual Status Init() = 0;

  virtual Status Allocate(int64_t size, StoreRegion* store_region) = 0;
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

class MemoryStore : public Store {
 public:
  MemoryStore(int64_t capacity);
  
  virtual Status Init();
  
  virtual Status Allocate(int64_t size, StoreRegion* store_region) override;
  virtual Status Free(StoreRegion* store_region) override;
  
  virtual int64_t GetCapacity()override { return capacity_; } 
  virtual int64_t GetFreeSize() override;
  virtual int64_t GetUsedSize() override;
  
  virtual std::string GetStoreName() override;

 private:
  int64_t capacity_;
  int64_t free_size_;
  int64_t used_size_;
};

class DCPMMStore : public Store {
 public:
  DCPMMStore(int64_t capacity);
  
  virtual Status Init();
  
  virtual Status Allocate(int64_t size, StoreRegion* store_region) override;
  virtual Status Free(StoreRegion* store_region) override;
  
  virtual int64_t GetCapacity()override { return capacity_; }
  virtual int64_t GetFreeSize() override;
  virtual int64_t GetUsedSize() override;
  
  virtual std::string GetStoreName() override;

 private:
  int64_t capacity_;
  int64_t free_size_;
  int64_t used_size_;
};

} // namespace pegasus

#endif  // PEGASUS_STORE_H