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

#include "rpc/server.h"
#include "dataset/request_identity.h"

namespace arrow {
class Status;
}  // namespace arrow

namespace pegasus {

namespace rpc {

class ServerCallContext;
class FlightServerBase;
class FlightMessageReader;
class FlightMetadataWriter;
class FlightDataStream;

struct Ticket;
}  //namespace rpc

class DatasetCacheManager;

class WorkerTableAPIService : public rpc::FlightServerBase {
 public:
  WorkerTableAPIService(std::shared_ptr<DatasetCacheManager> dataset_cache_manager);
  ~WorkerTableAPIService();

  Status Init();

  Status Serve();

  arrow::Status DoGet(const rpc::ServerCallContext& context, const rpc::Ticket& request,
               std::unique_ptr<rpc::FlightDataStream>* data_stream) override;

  arrow::Status DoPut(const rpc::ServerCallContext& context,
               std::unique_ptr<rpc::FlightMessageReader> reader,
               std::unique_ptr<rpc::FlightMetadataWriter> writer) override;


  virtual arrow::Status GetLocalData(const rpc::ServerCallContext& context,
                               const rpc::Ticket& request,
                               std::unique_ptr<rpc::LocalPartitionInfo>* response);
                                
  virtual arrow::Status ReleaseLocalData(const rpc::ServerCallContext& context,
                               const rpc::Ticket& request,
                               std::unique_ptr<rpc::LocalReleaseResult>* response);
private:
  std::shared_ptr<DatasetCacheManager> dataset_cache_manager_;
  WorkerExecEnv* env_;
  
  arrow::Status CreateDataRequest(const rpc::Ticket& request, RequestIdentity* request_identity);
};
    
}  // namespace pegasus

#endif  // PEGASUS_WORKER_TABLE_API_SERVICE_H