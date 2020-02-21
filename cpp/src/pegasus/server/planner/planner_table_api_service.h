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

#ifndef PEGASUS_PLANNER_TABLE_API_SERVICE_H
#define PEGASUS_PLANNER_TABLE_API_SERVICE_H

#include <string>

#include "dataset/dataset_request.h"
#include "dataset/dataset_service.h"
#include "rpc/server.h"
#include "storage/storage_plugin.h"

namespace pegasus {

class DataSetService;
class WorkerManager;

namespace rpc {

class ServerCallContext;
class FlightServerBase;
class FlightListing;
class FlightInfo;
class Criteria;
class FlightDescriptor;
class HeartbeatInfo;
class HeartbeatResult;

}  //namespace rpc

class PEGASUS_EXPORT PlannerTableAPIService : public rpc::FlightServerBase {
 public:
  PlannerTableAPIService(std::shared_ptr<DataSetService> dataset_service);
  ~PlannerTableAPIService();

  Status Init();

  Status Serve();

  arrow::Status ListFlights(const rpc::ServerCallContext& context, const rpc::Criteria* criteria,
                             std::unique_ptr<rpc::FlightListing>* listings) override;

  arrow::Status GetFlightInfo(const rpc::ServerCallContext& context,
                               const rpc::FlightDescriptor& request,
                               std::unique_ptr<rpc::FlightInfo>* info) override;
  
  arrow::Status Heartbeat(const rpc::ServerCallContext& context,
                               const rpc::HeartbeatInfo& request,
                               std::unique_ptr<rpc::HeartbeatResult>* response);
 private:
  std::shared_ptr<DataSetService> dataset_service_;
  std::shared_ptr<WorkerManager> worker_manager_;
  PlannerExecEnv* env_;

  arrow::Status CreateDataSetRequest(const rpc::FlightDescriptor& descriptor,
                                     DataSetRequest* dataset_request);
};

}  // namespace pegasus

#endif  // PEGASUS_PLANNER_TABLE_API_SERVICE_H