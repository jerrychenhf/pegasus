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

#include "pegasus/dataset/flightinfo_builder.h"

namespace pegasus {

FlightInfoBuilder::FlightInfoBuilder(std::unique_ptr<DataSet> dataset) : dataset_(std::move(dataset)){

}

Status FlightInfoBuilder::BuildFlightInfo(std::unique_ptr<FlightInfo>* flight_info) {
  std::unique_ptr<FlightDescriptor>* flight_descriptor;
  GetFlightDescriptor(flight_descriptor);
  std::unique_ptr<std::vector<FlightEndpoint>>* endpoints;
  GetFlightEndpoints(endpoints);
  int64_t* total_records;
  GetTotalRecords(total_records);
  int64_t* total_bytes;
  GetTotalBytes(total_bytes);

  arrow::flight::FlightInfo::Data flight_data;
  flight_data.descriptor = *flight_descriptor->get();
  flight_data.endpoints = *endpoints->get();
  flight_data.total_records = *total_records;
  flight_data.total_bytes = *total_bytes;
  arrow::flight::FlightInfo value(flight_data);
  *flight_info = std::unique_ptr<FlightInfo>(new FlightInfo(value));
}


Status BuildFlightInfo(std::unique_ptr<FlightInfo>* flight_info) {
  
}

Status FlightInfoBuilder::GetFlightDescriptor(std::unique_ptr<FlightDescriptor>* flight_descriptor) {

}

Status FlightInfoBuilder::GetFlightEndpoints(std::unique_ptr<std::vector<FlightEndpoint>>* endpoints) {

}


Status FlightInfoBuilder::GetTotalRecords(int64_t* total_records) {
    
}

Status FlightInfoBuilder::GetTotalBytes(int64_t* total_bytes) {
    
}

} // namespace pegasus
