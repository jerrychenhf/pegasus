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

#include "server/planner/planner_table_api_service.h"

namespace pegasus {
class Status;

namespace rpc {

class Location;

}  //namespace rpc

PlannerTableAPIService::PlannerTableAPIService(std::shared_ptr<DataSetService> dataset_service) {
  env_ =  ExecEnv::GetInstance();
  dataset_service_= dataset_service;
  worker_manager_ = env_->get_worker_manager();
}

PlannerTableAPIService::~PlannerTableAPIService() {
  
}

Status PlannerTableAPIService::Init() {

  std::string hostname = env_->GetPlannerGrpcHost();
  int32_t port = env_->GetPlannerGrpcPort();
  Location location;
  Location::ForGrpcTcp(hostname, port, &location);
  rpc::FlightServerOptions options(location);

  arrow::Status st = rpc::FlightServerBase::Init(options);
  if (!st.ok()) {
    return Status(StatusCode(st.code()), st.message());
  }
  return Status::OK();
}

Status PlannerTableAPIService::Serve() {
  arrow::Status st = rpc::FlightServerBase::Serve();
  if (!st.ok()) {
    return Status(StatusCode(st.code()), st.message());
  }
  return Status::OK();
}

/// \brief Retrieve a list of available fields given an optional opaque
/// criteria
/// \param[in] context The call context.
/// \param[in] criteria may be null
/// \param[out] listings the returned listings iterator
/// \return Status
arrow::Status PlannerTableAPIService::ListFlights(const rpc::ServerCallContext& context, const rpc::Criteria* criteria,
                     std::unique_ptr<rpc::FlightListing>* listings) {
    
  dataset_service_->GetFlightListing(listings);

  return arrow::Status::OK();
}

/// \brief Retrieve the schema and an access plan for the indicated
/// descriptor
/// \param[in] context The call context.
/// \param[in] request may be null
/// \param[out] info the returned flight info provider
/// \return Status
arrow::Status PlannerTableAPIService::GetFlightInfo(const rpc::ServerCallContext& context, const rpc::FlightDescriptor& request,
                       std::unique_ptr<rpc::FlightInfo>* out) {

  /*if (request.type == FlightDescriptor::PATH) {
    std::vector<std::string> request_path = request.path;
    if (request_path.size() != 1) {
      return arrow::Status::Invalid("Invalid path");
    }
    std::string dataset_path = request_path[0];

    dataset_service_->GetFlightInfo(dataset_path, out);

    arrow::Status::OK();
  }*/

  if (request.type == rpc::FlightDescriptor::CMD) {
    std::string request_table_name = request.cmd;
    auto parttftrs = std::make_shared<std::vector<Filter>>();
    // TODO: parse sql cmd here?

    Status st = dataset_service_->GetFlightInfo(request_table_name, parttftrs.get(), out);
    if (!st.ok()) {
      return arrow::Status(arrow::StatusCode(st.code()), st.message());
    }
    return arrow::Status::OK();
  } else {
    return arrow::Status::NotImplemented(request.type);
  }
}

arrow::Status PlannerTableAPIService::Heartbeat(const rpc::ServerCallContext& context,
  const rpc::HeartbeatInfo& request,
  std::unique_ptr<rpc::HeartbeatResult>* response) {
  Status status = worker_manager_->Heartbeat(request, response);
  return status.toArrowStatus();
}

}  // namespace pegasus