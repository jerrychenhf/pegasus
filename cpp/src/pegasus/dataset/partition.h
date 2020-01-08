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

#ifndef PEGASUS_ENDPOINT_H
#define PEGASUS_ENDPOINT_H

#include <vector>

#include "pegasus/dataset/identity.h"
#include "pegasus/common/location.h"

namespace pegasus {

/// \brief A identity and location where the identity can be redeemed
class Partition {
  ///  identify;
  Identity identity;

  /// The location where identity can be redeemed. 
  Location location;
public:
  Partition(Identity id): identity(id) {};
  Partition(Identity id, Location loc): identity(id), location(loc) {};
  ~Partition();

  bool Equals(const Partition& other) const;

  std::string GetIdentPath() {return identity.flie_path();}

  void UpdateLocation(Location lcn) {location = lcn;}

  friend bool operator==(const Partition& left, const Partition& right) {
    return left.Equals(right);
  }
  friend bool operator!=(const Partition& left, const Partition& right) {
    return !(left == right);
  }
};

} // namespace pegasus

#endif  // PEGASUS_ENDPOINT_H