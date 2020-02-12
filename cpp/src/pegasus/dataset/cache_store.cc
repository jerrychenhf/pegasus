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

#include "dataset/cache_store.h"
#include "common/logging.h"

using namespace std;

namespace pegasus {
  CacheStore::CacheStore(long capacity, std::shared_ptr<Store> store): capacity_(capacity), store_(store) {}
  CacheStore::~CacheStore(){}
  
  Status CacheStore::GetUsedSize(long& size) {
    size = used_size_;
    return Status::OK();
  }

  Status CacheStore::Allocate(long size, std::shared_ptr<CacheRegion>* cache_region) {
    if (store_ != NULL) {
      store_->Allocate(size, cache_region);
      return Status::OK();
    } else {
      stringstream ss;
      ss << "Failed to allocate cache region in current cache store. Because the store is NULL";
      LOG(ERROR) << ss.str();
      return Status::UnknownError(ss.str());
    }
    
  }

  Status CacheStore::Free(std::shared_ptr<CacheRegion> cache_region) {
    if (store_ != NULL) {
      store_->Free(cache_region);
      return Status::OK();
    } else {
      stringstream ss;
      ss << "Failed to free cache region in current cache store. Because the store is NULL";
      LOG(ERROR) << ss.str();
      return Status::UnknownError(ss.str());
    }
   
  }

  Status CacheStore::GetStore(std::shared_ptr<Store>* store) {
    if (store_ != NULL) {
      store = &store_;
      return Status::OK();
    } else {
      stringstream ss;
      ss << "Failed to get the store in current cache store. Because the store is NULL";
      LOG(ERROR) << ss.str();
      return Status::UnknownError(ss.str());
    }
  }
} // namespace pegasus