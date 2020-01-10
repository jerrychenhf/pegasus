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

#include "pegasus/runtime/exec_env.h"
#include "pegasus/server/worker/worker_table_api_service.h"

namespace pegasus {

namespace rpc {

class Location;

}  //namespace rpc

WorkerTableAPIService::WorkerTableAPIService() {
  env_ =  ExecEnv::GetInstance();
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
    return Status(StatusCode(st.code()), st.message());
  }
  return Status::OK();
}

Status WorkerTableAPIService::Serve() {
  arrow::Status st = rpc::FlightServerBase::Serve();
  if (!st.ok()) {
    return Status(StatusCode(st.code()), st.message());
  }
}

/// \brief Get a stream of IPC payloads to put on the wire
/// \param[in] context The call context.
/// \param[in] request an opaque ticket+
/// \param[out] stream the returned stream provider
/// \return Status
arrow::Status WorkerTableAPIService::DoGet(const rpc::ServerCallContext& context, const rpc::Ticket& request,
               std::unique_ptr<rpc::FlightDataStream>* data_stream) {
    
  std::unique_ptr<arrow::RecordBatch> chunk;

  //TODO Ticket=>Identity
  // request => identity

  //cache_manager_.GetFlightDataStream(identity, data_stream);
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

}  // namespace pegasus