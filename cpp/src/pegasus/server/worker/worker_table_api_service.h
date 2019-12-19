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

#ifndef PEGASUS_WORKER_TABLE_API_SERVICE_H
#define PEGASUS_WORKER_TABLE_API_SERVICE_H

#include "arrow/flight/server.h"

#include "pegasus/cache/cache_manager.h"


namespace arrow {

class Status;

}  // namespace ipc

namespace pegasus {

class WorkerTableAPIService : public arrow::flight::FlightServerBase {
 public:
  WorkerTableAPIService();
  ~WorkerTableAPIService();

  Status Init();

  arrow::Status DoGet(const ServerCallContext& context, const Ticket& request,
               std::unique_ptr<FlightDataStream>* data_stream) override;

  arrow::Status DoPut(const ServerCallContext& context,
               std::unique_ptr<FlightMessageReader> reader,
               std::unique_ptr<FlightMetadataWriter> writer) override;

private:
  std::shared_ptr<CacheManager> cache_manager_;

};
    
}  // namespace pegasus

#endif  // PEGASUS_WORKER_TABLE_API_SERVICE_H