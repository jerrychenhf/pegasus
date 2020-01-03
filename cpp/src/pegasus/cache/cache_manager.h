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

#ifndef PEGASUS_CACHE_MANAGER_H
#define PEGASUS_CACHE_MANAGER_H

#include "arrow/record_batch.h"
#include "arrow/flight/server.h"

#include "pegasus/dataset/identity.h"
#include "pegasus/common/location.h"
#include "pegasus/storage/storage_plugin.h"
#include "pegasus/cache/store_manager.h"


using namespace std;
using namespace arrow;
using namespace arrow::flight;

namespace pegasus {

class CacheManager {
 public:
  CacheManager();
  ~CacheManager();

  Status GetTableChunk(Identity identity, std::unique_ptr<RecordBatch> out);
  Status InsertTableChunk(std::shared_ptr<RecordBatch> in);
  Status GetFlightDataStream(Identity Identity, std::unique_ptr<FlightDataStream>* data_stream);

 private: 
  std::shared_ptr<Location> location_;
  std::shared_ptr<StoragePlugin> storage_plugin_;
  std::shared_ptr<StoreManager> store_manager_;
};

} // namespace pegasus

#endif  // PEGASUS_CACHE_MANAGER_H