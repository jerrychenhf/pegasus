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
  env_ =  PlannerExecEnv::GetInstance();
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

arrow::Status PlannerTableAPIService::GetSchema(const rpc::ServerCallContext& context,
                                                const rpc::FlightDescriptor& request,
                                                std::unique_ptr<rpc::SchemaResult>* schema) {

  DataSetRequest dataset_request;
  LOG(INFO) << "Requesting schema.";
  arrow::Status st = CreateDataSetRequest(request, &dataset_request);

  if (!st.ok()) {
    return st;
  }
  std::unique_ptr<rpc::FlightInfo> info;
  Status status = dataset_service_->GetFlightInfo(&dataset_request, &info, request);
  if (!st.ok()) {
    return st;
  }
  *schema = std::unique_ptr<rpc::SchemaResult>(
    new rpc::SchemaResult(info->serialized_schema()));
  return arrow::Status::OK();
}

/// \brief Retrieve a list of available fields given an optional opaque
/// criteria
/// \param[in] context The call context.
/// \param[in] criteria may be null
/// \param[out] listings the returned listings iterator
/// \return Status
arrow::Status PlannerTableAPIService::ListFlights(const rpc::ServerCallContext& context,
                                                  const rpc::Criteria* criteria,
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
arrow::Status PlannerTableAPIService::GetFlightInfo(const rpc::ServerCallContext& context,
                                                    const rpc::FlightDescriptor& request,
                                                    std::unique_ptr<rpc::FlightInfo>* out) {

  DataSetRequest dataset_request;
  LOG(INFO) << "Requesting flight info";
  arrow::Status st = CreateDataSetRequest(request, &dataset_request);

  if (!st.ok()) {
    return st;
  }
  Status status = dataset_service_->GetFlightInfo(&dataset_request, out, request);
  return status.toArrowStatus();
}

arrow::Status PlannerTableAPIService::Heartbeat(const rpc::ServerCallContext& context,
  const rpc::HeartbeatInfo& request,
  std::unique_ptr<rpc::HeartbeatResult>* response) {
  Status status = worker_manager_->Heartbeat(request, response);
  return status.toArrowStatus();
}

/// \brief Convert FlightDescriptor request to DataSetRequest
/// \param[in] flight descriptor.
/// \param[out] dataset request.
/// \return Status
arrow::Status PlannerTableAPIService::CreateDataSetRequest(const rpc::FlightDescriptor& descriptor,
                                                           DataSetRequest* dataset_request) {
  
  if (descriptor.type == rpc::FlightDescriptor::PATH) {
    std::vector<std::string> request_path = descriptor.path;
    if (request_path.size() != 1) {
      return arrow::Status::Invalid("Invalid dataset path, currently only supports one path");
    }
    std::string dataset_path = request_path[0];
    LOG(INFO) << "dataset path:" << dataset_path;
    dataset_request->set_dataset_path(dataset_path);
  } else {
    return arrow::Status::NotImplemented(descriptor.type);
  }

  if(!descriptor.properties.empty()) {
    dataset_request->set_properties(descriptor.properties);
  }
  return arrow::Status::OK();
}

}  // namespace pegasus