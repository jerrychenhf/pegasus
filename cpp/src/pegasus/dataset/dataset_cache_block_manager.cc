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

Status DatasetCacheBlockManager::GetCachedDataSet(Identity* identity, std::shared_ptr<CachedDataset>* datasets) {
  std::string dataset_path = identity->dataset_path();
  auto entry = cached_datasets_.find(dataset_path);

  if (entry == cached_datasets_.end()) {
      return Status::KeyError("Could not find the cached dataset.", dataset_path);
  }
  auto find_cache_info = entry->second;
  *datasets = std::shared_ptr<CachedDataset>(find_cache_info);
  return Status::OK();
}


Status DatasetCacheBlockManager::GetCachedPartition(Identity* identity, std::shared_ptr<std::vector<CachedPartition>>* partitions) {
  std::string dataset_path = identity->dataset_path();
  std::shared_ptr<CachedDataset>* dataset;
  GetCachedDataSet(identity, dataset);
  std::shared_ptr<std::vector<CachedPartition>> cached_partitions = (*dataset)->partitions();
  for (auto iter = cached_partitions->begin(); iter != cached_partitions->end(); iter++)
	{
		partitions->get()->push_back(*iter);
	}
}

Status DatasetCacheBlockManager::InsertPartition(Identity* identity, std::shared_ptr<CachedPartition> new_partition) {
  std::string dataset_path = identity->dataset_path();
  std::shared_ptr<CachedDataset>* dataset;
  GetCachedDataSet(identity, dataset);
  std::shared_ptr<std::vector<CachedPartition>> partitions = (*dataset)->partitions();
  partitions->push_back(*new_partition);
}

} // namespace pegasus