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

#include "cache/cache_store.h"
#include "common/logging.h"

using namespace std;

namespace pegasus {
  CacheStore::CacheStore(Store* store, int64_t capacity): store_(store), capacity_(capacity) {}
  CacheStore::~CacheStore(){}
  

  Status CacheStore::Allocate(int64_t size, StoreRegion* store_region) {
    if (store_ == nullptr) {
      stringstream ss;
      ss << "Failed to allocate cache region in current cache store. Because the store is NULL";
      LOG(ERROR) << ss.str();
      return Status::UnknownError(ss.str());
    }
    
    Status status = store_->Allocate(size, store_region);
    if (!status.ok()) {
      stringstream ss;
      ss << "Failed to allocate cache region in current cache store";
      LOG(ERROR) << ss.str();
      return Status::UnknownError(ss.str());
    }
    
    used_size_ += size;
    return Status::OK();
  }

  Status CacheStore::Free(StoreRegion* store_region) {
    if (store_ == nullptr) {
      stringstream ss;
      ss << "Failed to free cache region in current cache store. Because the store is NULL";
      LOG(ERROR) << ss.str();
      return Status::UnknownError(ss.str());
    }
    
    int64_t size = store_region->length();
    Status status = store_->Free(store_region);
    if (!status.ok()) {
      stringstream ss;
      ss << "Failed to free cache region in current cache store";
      LOG(ERROR) << ss.str();
      return Status::UnknownError(ss.str());
    }
    used_size_ -= size;
    return Status::OK();
  }
} // namespace pegasus