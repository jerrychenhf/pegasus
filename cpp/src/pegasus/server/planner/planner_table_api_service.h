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

#include "arrow/flight/server.h"

#include "pegasus/dataset/dataset_service.h"
#include "pegasus/storage/storage_plugin.h"

namespace pegasus {

class PlannerTableAPIService : public arrow::flight::FlightServerBase {
 public:
  PlannerTableAPIService(std::shared_ptr<DataSetService> dataset_service);
  ~PlannerTableAPIService();

  Status Init();

  arrow::Status ListFlights(const ServerCallContext& context, const Criteria* criteria,
                             std::unique_ptr<FlightListing>* listings) override;

  arrow::Status GetFlightInfo(const ServerCallContext& context,
                               const FlightDescriptor& request,
                               std::unique_ptr<FlightInfo>* info) override;

 private:
  std::shared_ptr<DataSetService> dataset_service_;
  std::shared_ptr<WorkerManager> worker_manager_;
};

}  // namespace pegasus

#endif  // PEGASUS_PLANNER_TABLE_API_SERVICE_H