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

#include "pegasus/dataset/dataset_cache_block_manager.h"

using namespace pegasus;

namespace pegasus {

DatasetCacheBlockManager::DatasetCacheBlockManager() {
}

DatasetCacheBlockManager::~DatasetCacheBlockManager() {}

Status DatasetCacheBlockManager::GetCachedRecord(Identity identity, std::shared_ptr<CachedInfo>* cached_dataset) {
  auto entry = cached_datasets_.find(identity.flie_path());
  if (entry == cached_datasets_.end()) {
      return Status::KeyError("Could not find the cached info.", identity.flie_path());
  }

  auto find_cache_info = entry->second;
  *cached_dataset = std::shared_ptr<CachedInfo>(find_cache_info);
  return Status::OK();
}

Status DatasetCacheBlockManager::InsertCachedRecord(Identity identity) {
    // insert the column ;
    // 1. search the dataset of file path, if not create, create and inserte;
    // 2. If exist, insert the record to the existed dataset.
}

} // namespace pegasus