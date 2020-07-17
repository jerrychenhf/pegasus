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

#include "runtime/worker_exec_env.h"
#include "server/worker/worker_table_api_service.h"
#include "dataset/dataset_cache_manager.h"

namespace pegasus {

namespace rpc {

class Location;

}  //namespace rpc

WorkerTableAPIService::WorkerTableAPIService(std::shared_ptr<DatasetCacheManager> dataset_cache_manager)
  : dataset_cache_manager_(dataset_cache_manager) {
  env_ =  WorkerExecEnv::GetInstance();
}

WorkerTableAPIService::~WorkerTableAPIService() {
  
}

Status WorkerTableAPIService::Init() {
  std::string hostname = env_->GetWorkerGrpcHost();
  int32_t port = env_->GetWorkerGrpcPort();

  pegasus::rpc::Location location;
  pegasus::rpc::Location::ForGrpcTcp(hostname, port, &location);
  pegasus::rpc::FlightServerOptions options(location);
  arrow::Status st = rpc::FlightServerBase::Init(options);
  if (!st.ok()) {
    return Status::fromArrowStatus(st);
  }
  return Status::OK();
}

Status WorkerTableAPIService::Serve() {
  arrow::Status st = rpc::FlightServerBase::Serve();
  if (!st.ok()) {
    return Status::fromArrowStatus(st);
  }
  return Status::OK();
}

/// \brief Get a stream of IPC payloads to put on the wire
/// \param[in] context The call context.
/// \param[in] request an opaque ticket+
/// \param[out] stream the returned stream provider
/// \return Status
arrow::Status WorkerTableAPIService::DoGet(const rpc::ServerCallContext& context,
                                           const rpc::Ticket& request,
                                           std::unique_ptr<rpc::FlightDataStream>* data_stream) {

  RequestIdentity request_identity;
  arrow::Status st = CreateDataRequest(request, &request_identity);

  if(!st.ok())
    return st;
  
  Status status = dataset_cache_manager_->GetDatasetStream(&request_identity, data_stream);
 
  return status.toArrowStatus();
}

/// \brief Process a stream of IPC payloads sent from a client
/// \param[in] context The call context.
/// \param[in] reader a sequence of uploaded record batches
/// \param[in] writer send metadata back to the client
/// \return Status
arrow::Status WorkerTableAPIService::DoPut(const rpc::ServerCallContext& context,
                                           std::unique_ptr<rpc::FlightMessageReader> reader,
                                           std::unique_ptr<rpc::FlightMetadataWriter> writer) {

//TODO
  return arrow::Status::OK();
}


arrow::Status WorkerTableAPIService::GetLocalData(const rpc::ServerCallContext& context,
                               const rpc::Ticket& request,
                               std::unique_ptr<rpc::LocalPartitionInfo>* response) {

  RequestIdentity request_identity;
  arrow::Status st = CreateDataRequest(request, &request_identity);

  if(!st.ok())
    return st;

  Status status = dataset_cache_manager_->GetLocalData(&request_identity, response);

  return arrow::Status::OK();
}
                                

arrow::Status WorkerTableAPIService::ReleaseLocalData(const rpc::ServerCallContext& context,
                               const rpc::Ticket& request,
                               std::unique_ptr<rpc::LocalReleaseResult>* response) {

  RequestIdentity request_identity;
  arrow::Status st = CreateDataRequest(request, &request_identity);

  if(!st.ok())
    return st;

  Status status = dataset_cache_manager_->ReleaseLocalData(&request_identity, response);
  
  return arrow::Status::OK();
}

arrow::Status WorkerTableAPIService::CreateDataRequest(const rpc::Ticket& request,
  RequestIdentity* request_identity) {

  *request_identity = RequestIdentity(
    request.dataset_path, request.partition_identity, request.column_indices);
  
  std::shared_ptr<arrow::Schema> schema;
  arrow::Status status = request.GetSchema(&schema);
  if(!status.ok()) {
    return status;
  }
  request_identity->set_schema(schema);
  return arrow::Status::OK();
}

}  // namespace pegasus