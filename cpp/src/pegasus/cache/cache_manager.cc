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

#include <memory>
#include <unordered_map>

#include "pegasus/cache/cache_manager.h"
#include "pegasus/cache/store_factory.h"
#include "pegasus/storage/storage_plugin_factory.h"
#include "pegasus/runtime/exec_env.h"

namespace pegasus {

CacheManager::CacheManager() {
  ExecEnv* env =  ExecEnv::GetInstance();
  std::shared_ptr<StoragePluginFactory> storage_plugin_factory = env->get_storage_plugin_factory();
  storage_plugin_factory->GetStoragePlugin(env->GetOptions()->storage_plugin_type_, storage_plugin_);
}

// if ticket is not exist,connect to storage with file location and row group index to get table chunk,
// store the table chunk to worker cache
// if exist, return the appropriate table chunk given the ticket
Status CacheManager::GetTableChunk(Identity identity, std::unique_ptr<RecordBatch> out) {

}
Status CacheManager::InsertTableChunk(std::shared_ptr<RecordBatch> in) {

}

Status CacheManager::GetFlightDataStream(Identity Identity, std::unique_ptr<FlightDataStream>* data_stream) {

}

} // namespace pegasus