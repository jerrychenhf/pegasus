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

#include "pegasus/server/planner/planner_table_api_service.h"

namespace pegasus {

PlannerTableAPIService::PlannerTableAPIService() {

}

PlannerTableAPIService::~PlannerTableAPIService() {
  
}

Status PlannerTableAPIService::Init() {
  ExecEnv* env =  ExecEnv::GetInstance();
  //TODO
  //FlightServerBase::Init(env->GetOptions);
  worker_manager_ = env->get_worker_manager();
  dataset_service_ = std::unique_ptr<DataSetService>(new DataSetService());
}

/// \brief Retrieve a list of available fields given an optional opaque
/// criteria
/// \param[in] context The call context.
/// \param[in] criteria may be null
/// \param[out] listings the returned listings iterator
/// \return Status
arrow::Status PlannerTableAPIService::ListFlights(const ServerCallContext& context, const Criteria* criteria,
                     std::unique_ptr<FlightListing>* listings) {
    
  dataset_service_->GetFlightListing(listings);

  return arrow::Status::OK();
}

/// \brief Retrieve the schema and an access plan for the indicated
/// descriptor
/// \param[in] context The call context.
/// \param[in] request may be null
/// \param[out] info the returned flight info provider
/// \return Status
arrow::Status PlannerTableAPIService::GetFlightInfo(const ServerCallContext& context, const FlightDescriptor& request,
                       std::unique_ptr<FlightInfo>* out) {
  if (request.type == FlightDescriptor::PATH) {
    std::vector<std::string> request_path = request.path;
    if (request_path.size() != 1) {
      return arrow::Status::Invalid("Invalid path");
    }
    std::string dataset_path = request_path[0];

    dataset_service_->GetFlightInfo(dataset_path, out);

    arrow::Status::OK();
  } else {
    return arrow::Status::NotImplemented(request.type);
  }
}

}  // namespace pegasus