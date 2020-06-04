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

#ifndef PEGASUS_STORE_MANAGER_H
#define PEGASUS_STORE_MANAGER_H

#include <unordered_map>
#include "cache/store.h"

namespace pegasus {

class StoreManager {
 public:
  StoreManager();
  ~StoreManager();

  Status Init();

  Status GetStore(const std::string& id, Store** store);
  Status GetStore(Store::StoreType cache_type, Store** store);

  std::unordered_map<std::string, std::shared_ptr<Store>> GetStores() {
    return stores_;
  }

 private:
  std::unordered_map<std::string, std::shared_ptr<Store>> stores_;
  
public:
  static const std::string STORE_ID_DRAM;
  static const std::string STORE_ID_DCPMM;
  static const std::string STORE_PROPERTY_PATH;
  static const std::string STORE_ID_FILE;
  
  static const int64_t KILOBYTE = 1024;
  static const int64_t MEGABYTE = KILOBYTE * 1024;
  static const int64_t GIGABYTE = MEGABYTE * 1024;  
};

} // namespace pegasus

#endif  // PEGASUS_STORE_MANAGER_H