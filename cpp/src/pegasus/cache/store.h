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

#include "pegasus/common/status.h"
#include "pegasus/cache/cache_region.h"

namespace pegasus {

class Store {
 public:

  virtual Status Allocate(long size, std::shared_ptr<CacheRegion>* cache_entry_holder) = 0;
  virtual Status GetTotalSize(long& total_size) = 0;
  virtual Status GetUsedSize(long& used_size) = 0;
  virtual std::string GetStoreName() = 0;

  enum StoreType {
    MEMORY,
    DCPMM,
    FILE
  };
    
  enum CacheReplacePolicy {
    LRU,
    MRU
  };

 private:
  long total_size_;
  long used_size_;
};

class MemoryStore : public Store {
 public:
  MemoryStore();
  Status Allocate(long size, std::shared_ptr<CacheRegion>* cache_entry_holder) override;
  Status GetTotalSize(long& total_size) override;
  Status GetUsedSize(long& used_size) override;
  std::string GetStoreName() override;

};

class DCPMMStore : public Store {
 public:
  DCPMMStore();
  Status Allocate(long size, std::shared_ptr<CacheRegion>* cache_entry_holder) override;
  Status GetTotalSize(long& total_size) override;
  Status GetUsedSize(long& used_size) override;
  std::string GetStoreName() override;
};

// class FileStore : public Store {
//  public:
//   FileStore();
//   // Status Allocate(long size) override;
//   // Status GetTotalSize(long& total_size) override;
//   // Status GetUsedSize(long& used_size) override;
//   // std::string GetStoreName() override;
// };

} // namespace pegasus

#endif  // PEGASUS_STORE_H