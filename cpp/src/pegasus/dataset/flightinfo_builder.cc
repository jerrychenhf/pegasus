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
#include "rpc/types.h"

namespace pegasus {

FlightInfoBuilder::FlightInfoBuilder(std::shared_ptr<ResultDataSet> dataset) : dataset_(dataset){

}

FlightInfoBuilder::FlightInfoBuilder(std::shared_ptr<std::vector<std::shared_ptr<ResultDataSet>>> datasets) {

}

Status FlightInfoBuilder::BuildFlightInfo(std::unique_ptr<rpc::FlightInfo>* flight_info) {
  std::unique_ptr<rpc::FlightDescriptor> flight_descriptor;
  GetFlightDescriptor(&flight_descriptor);
  std::unique_ptr<std::vector<rpc::FlightEndpoint>> endpoints;
  GetFlightEndpoints(&endpoints);
  int64_t* total_records;
  GetTotalRecords(total_records);
  int64_t* total_bytes;
  GetTotalBytes(total_bytes);

  rpc::FlightInfo::Data flight_data;
  flight_data.descriptor = *flight_descriptor;
  flight_data.endpoints = *endpoints;
  flight_data.total_records = *total_records;
  flight_data.total_bytes = *total_bytes;
  rpc::FlightInfo value(flight_data);
  *flight_info = std::unique_ptr<rpc::FlightInfo>(new rpc::FlightInfo(value));
  return Status::OK();
}


Status FlightInfoBuilder::BuildFlightListing(std::unique_ptr<rpc::FlightListing>* listings) {
  
  return Status::OK();
}

Status FlightInfoBuilder::GetFlightDescriptor(std::unique_ptr<rpc::FlightDescriptor>* flight_descriptor) {

  return Status::OK();
}

Status FlightInfoBuilder::GetFlightEndpoints(std::unique_ptr<std::vector<rpc::FlightEndpoint>>* endpoints) {
  // fill ticket and locations in the endpoints
  // Ticket ticket;    std::string dataset_path;  std::string partition_identity;  std::vector<int> column_indices;
  // std::vector<Location> locations;
  rpc::Ticket tkt;
  tkt.setDatasetpath(dataset_->dataset_path());
//  tkt.setPartitionid(dataset_->);
//  tkt.set
  rpc::FlightEndpoint fep;

  (*endpoints)->push_back(fep);

  return Status::OK();
}

Status FlightInfoBuilder::GetTotalRecords(int64_t* total_records) {
    
  return Status::OK();
}

Status FlightInfoBuilder::GetTotalBytes(int64_t* total_bytes) {
    
  return Status::OK();
}

} // namespace pegasus
