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

#include "dataset/flightinfo_builder.h"
#include "dataset/partition.h"
#include "rpc/types.h"
#include "rpc/internal.h"

namespace pegasus {

FlightInfoBuilder::FlightInfoBuilder(
  std::shared_ptr<ResultDataSet> dataset) : dataset_(dataset){
}

FlightInfoBuilder::FlightInfoBuilder(
  std::shared_ptr<std::vector<std::shared_ptr<ResultDataSet>>> datasets) {
}

Status FlightInfoBuilder::BuildFlightInfo(std::unique_ptr<rpc::FlightInfo>* flight_info,
                                          std::shared_ptr<arrow::Schema> schema,
                                          std::vector<int32_t>& indices,
                                          rpc::FlightDescriptor& fldtr) {

  LOG(INFO) << "BuildFlightInfo()...";
  std::unique_ptr<std::vector<rpc::FlightEndpoint>> endpoints;
  GetFlightEndpoints(&endpoints, schema, indices);

  std::string schema_string;
  rpc::internal::SchemaToString(*(dataset_->get_schema()), &schema_string);

  rpc::FlightInfo::Data flight_data;
  flight_data.schema = schema_string;
  flight_data.descriptor = fldtr;
  flight_data.endpoints = *endpoints;
  flight_data.total_records = GetTotalRecords();
  flight_data.total_bytes = GetTotalBytes();

  *flight_info  = std::unique_ptr<rpc::FlightInfo>(new rpc::FlightInfo(std::move(flight_data)));

  LOG(INFO) << "FlightInfoBuilder::BuildFlightInfo() finished successfully.";
  return Status::OK();
}

Status FlightInfoBuilder::BuildFlightListing(std::unique_ptr<rpc::FlightListing>* listings) {
  return Status::OK();
}

Status FlightInfoBuilder::GetFlightDescriptor(std::unique_ptr<rpc::FlightDescriptor> flight_descriptor) {
  return Status::OK();
}

Status FlightInfoBuilder::GetFlightEndpoints(std::unique_ptr<std::vector<rpc::FlightEndpoint>>* endpoints,
                                             std::shared_ptr<arrow::Schema> schema,
                                             std::vector<int32_t>& indices) {

  LOG(INFO) << "GetFlightEndpoints()...";
  // fill ticket and locations in the endpoints
  // every endpoint has 1 ticket + many locations
  // every ticket has 1 dataset_path, 1 partition_id, many column indices.
  // so, every endpoint = 1 dataset_path + 1 partition_id + many column indices + many locations.
  // every dataset has 1 schema, 1 dataset_path, many partitions.
  // every partition has 1 identity, 1 location.
  // every identity has 1 dataset_path, 1 partition_id.
  // so, every dataset has 1 schema + 1 dataset_path + many partition ids + many locations.

  *endpoints = std::unique_ptr<std::vector<rpc::FlightEndpoint>>(new std::vector<rpc::FlightEndpoint>());

  for (auto partition:dataset_->partitions())
  {
    rpc::FlightEndpoint endpoint;
    std::string schema_string;
    rpc::internal::SchemaToString(*schema, &schema_string);
    endpoint.ticket.setSchema(schema_string);
    endpoint.ticket.setDatasetpath(dataset_->dataset_path());
    endpoint.ticket.setPartitionid(partition.GetIdentityPath());
    endpoint.ticket.setColids(indices);
    endpoint.locations.push_back(partition.GetLocation());

    (*endpoints)->push_back(endpoint);
  }

  LOG(INFO) << "...GetFlightEndpoints()";
  return Status::OK();
}

int64_t FlightInfoBuilder::GetTotalRecords() {  
  return dataset_->total_records();
}

int64_t FlightInfoBuilder::GetTotalBytes() {
  return dataset_->total_bytes();
}

} // namespace pegasus
