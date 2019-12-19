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

#include "arrow/flight/types.h"

#include "pegasus/dataset/dataset.h"

using namespace arrow;
using namespace arrow::flight;

namespace pegasus {

class FlightInfoBuilder {

public:
  FlightInfoBuilder(std::unique_ptr<DataSet> dataset);
  FlightInfoBuilder(std::unique_ptr<std::vector<std::shared_ptr<DataSet>>> datasets);

  Status BuildFlightInfo(std::unique_ptr<FlightInfo>* flight_info);

  Status BuildFlightListing(std::unique_ptr<FlightListing>* listings);

  Status GetFlightDescriptor(std::unique_ptr<FlightDescriptor>* flight_descriptor);

  Status GetFlightEndpoints(std::unique_ptr<std::vector<FlightEndpoint>>* endpoints);

  Status GetTotalRecords(int64_t* total_records);
    
  Status GetTotalBytes(int64_t* total_bytes);

private:
  std::unique_ptr<DataSet> dataset_;
  std::unique_ptr<std::vector<std::shared_ptr<DataSet>>> datasets_;
};

} // namespace pegasus

#endif  // PEGASUS_FLIGHTINFO_BUILDER_H