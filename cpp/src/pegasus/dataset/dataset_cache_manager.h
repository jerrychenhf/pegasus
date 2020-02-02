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

#ifndef PEGASUS_DATASET_CACHE_MANAGER_H
#define PEGASUS_DATASET_CACHE_MANAGER_H

#include "arrow/record_batch.h"
#include "arrow/flight/server.h"

#include "pegasus/dataset/identity.h"
#include "pegasus/dataset/dataset_cache_block_manager.h"
#include "pegasus/dataset/dataset_cache_engine_manager.h"
#include "pegasus/storage/storage_plugin.h"
#include "pegasus/storage/storage_plugin_factory.h"

using namespace std;
using namespace arrow;
using namespace arrow::flight;

namespace pegasus {

class DatasetCacheManager {
 public:
  DatasetCacheManager();
  ~DatasetCacheManager();

  Status GetDatasetStream(Identity* identity, std::unique_ptr<FlightDataStream>* data_stream);
  static CacheEngine::CachePolicy GetCachePolicy(Identity* identity);

 private: 
  std::shared_ptr<DatasetCacheBlockManager> dataset_cache_block_manager_;
  std::shared_ptr<DatasetCacheEngineManager> dataset_cache_engine_manager_;
  std::shared_ptr<StoragePluginFactory> storage_plugin_factory_;
};

} // namespace pegasus

#endif  // PEGASUS_DATASET_CACHE_MANAGER_H