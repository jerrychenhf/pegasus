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

#ifndef PEGASUS_FLIGHTINFO_BUILDER_H
#define PEGASUS_FLIGHTINFO_BUILDER_H

#include "rpc/types.h"

#include "dataset/dataset.h"

namespace pegasus {

namespace rpc {

class FlightInfo;
class FlightListing;
struct FlightDescriptor;
struct FlightEndpoint;

}  //namespace rpc

class FlightInfoBuilder {

public:
  FlightInfoBuilder(std::shared_ptr<ResultDataSet> dataset);
  FlightInfoBuilder(std::shared_ptr<std::vector<std::shared_ptr<ResultDataSet>>> datasets);

  Status BuildFlightInfo(std::unique_ptr<rpc::FlightInfo>* flight_info,
                         std::shared_ptr<arrow::Schema> schema, std::vector<int32_t>& indices, rpc::FlightDescriptor& fldtr);

  Status BuildFlightListing(std::unique_ptr<rpc::FlightListing>* listings);

  Status GetFlightDescriptor(std::unique_ptr<rpc::FlightDescriptor> flight_descriptor);

  Status GetFlightEndpoints(std::unique_ptr<std::vector<rpc::FlightEndpoint>>* endpoints,
      std::shared_ptr<arrow::Schema> schema,
      std::vector<int32_t>& indices);

  int64_t GetTotalRecords();
  int64_t GetTotalBytes();

private:
  std::shared_ptr<ResultDataSet> dataset_;
  std::shared_ptr<std::vector<std::shared_ptr<ResultDataSet>>> datasets_;
};

} // namespace pegasus

#endif  // PEGASUS_FLIGHTINFO_BUILDER_H