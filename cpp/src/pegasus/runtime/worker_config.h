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

#ifndef PEGASUS_WORKER_CONFIG_H
#define PEGASUS_WORKER_CONFIG_H

#include <unordered_map>
#include "cache/store.h"
#include "cache/cache_engine.h"

using namespace std;

namespace pegasus {

typedef std::unordered_map<string, string> StoreProperties;
    
class StoreInfo {
public:
  StoreInfo(const std::string& id, Store::StoreType type, int64_t capacity,
    const std::shared_ptr<StoreProperties>& properties)
    : id_(id), type_(type), capacity_(capacity), properties_(properties) {
  }
  
  ~StoreInfo() {
  }
  
  const std::string& id() { return id_; }
  Store::StoreType type() { return type_; }
  int64_t capacity() const { return capacity_; }
  std::shared_ptr<StoreProperties> properties() { return properties_; }
private:
  std::string id_;
  Store::StoreType type_;
  int64_t capacity_;  
  std::shared_ptr<StoreProperties> properties_;
};

typedef std::unordered_map<string, std::shared_ptr<StoreInfo>> StoreInfos;

// cache store configurations
typedef std::unordered_map<string, string> CacheStoreProperties;
    
class CacheStoreInfo {
public:
  CacheStoreInfo(const std::string& id, const std::string& store_id, int64_t capacity_quote,
    const std::shared_ptr<CacheStoreProperties>& properties)
    : id_(id), store_id_(store_id), capacity_quote_(capacity_quote), properties_(properties) {
  }
  
  ~CacheStoreInfo() {
  }
  
  const std::string& id() { return id_; }
  const std::string& store_id() { return store_id_; }
  int64_t capacity_quote() const { return capacity_quote_; }
  std::shared_ptr<CacheStoreProperties> properties() { return properties_; }
private:
  std::string id_;
  std::string store_id_;
  int64_t capacity_quote_;  
  std::shared_ptr<CacheStoreProperties> properties_;
};

typedef std::unordered_map<string, std::shared_ptr<CacheStoreInfo>> CacheStoreInfos;

typedef std::unordered_map<string, string> CacheEngineProperties;
    
class CacheEngineInfo {
public:
  CacheEngineInfo(const std::string& id, CacheEngine::CachePolicy type, int64_t capacity,
    const std::shared_ptr<CacheEngineProperties>& properties)
    : id_(id), type_(type), capacity_(capacity), properties_(properties) {
  }
  
  ~CacheEngineInfo() {
  }
  
  const std::string& id() { return id_; }
  CacheEngine::CachePolicy type() { return type_; }
  int64_t capacity() const { return capacity_; }
  std::shared_ptr<CacheEngineProperties> properties() { return properties_; }
  const CacheStoreInfos& cache_stores() const { return cache_store_infos_; }
  
  void add_cache_store(const std::shared_ptr<CacheStoreInfo>& cache_store_info) {
    cache_store_infos_.insert(
      std::make_pair(cache_store_info->id(), cache_store_info));
  }
private:
  std::string id_;
  CacheEngine::CachePolicy type_;
  int64_t capacity_;  
  std::shared_ptr<CacheEngineProperties> properties_;
  CacheStoreInfos cache_store_infos_;
};

typedef std::unordered_map<string, std::shared_ptr<CacheEngineInfo>> CacheEngineInfos;

} // namespace pegasus

#endif  // PEGASUS_WORKER_CONFIG_H