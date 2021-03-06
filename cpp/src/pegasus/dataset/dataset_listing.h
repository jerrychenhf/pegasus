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

#ifndef PEGASUS_DATASET_LISTING_H
#define PEGASUS_DATASET_LISTING_H

#include<arrow/flight/types.h>

using namespace arrow;
using namespace arrow::flight;

namespace pegasus {

/// \brief A FlightListing implementation based on a vector of
/// FlightInfo objects.
///
/// This can be iterated once, then it is consumed.
class DataSetListing : public FlightListing {
 public:
  explicit DataSetListing(const std::vector<FlightInfo>& flights);
  explicit DataSetListing(std::vector<FlightInfo>&& flights);

  Status Next(std::unique_ptr<FlightInfo>* info) override;

 private:
  int position_;
  std::vector<FlightInfo> flights_;
};

} // namespace pegasus

#endif  // PEGASUS_DATASET_LISTING_H