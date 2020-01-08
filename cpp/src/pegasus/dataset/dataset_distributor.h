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

#ifndef PEGASUS_DATASET_DISTRIBUTOR_H
#define PEGASUS_DATASET_DISTRIBUTOR_H

#include <string>
#include <vector>

#include "pegasus/common/location.h"
#include "pegasus/dataset/identity.h"
#include "pegasus/dataset/partition.h"
#include "pegasus/util/conhash.h"

using namespace std;

namespace pegasus {

enum {
  CONHASH,
  LOCALONLY,
  LOCALPREFER
};

  class DSDistributor {
  public:
    DSDistributor();
    ~DSDistributor();
    virtual void PrepareValidLocations(std::shared_ptr<std::vector<std::shared_ptr<Location>>> locations);
    virtual void SetupDist();
    void AddLocation(Location location);
    void AddLocation(Location location, int num_virtual_nodes);
    void RemoveLocation(Location location);
    Location GetLocation(Identity identity);
    std::string GetHash(std::string key);
    virtual void GetDistLocations(std::shared_ptr<std::vector<Identity>> vectident, std::shared_ptr<std::vector<Location>> vectloc);
    virtual void GetDistLocations(std::shared_ptr<std::vector<Partition>> partitions);
//  private:
	  int distpolicy_;
    std::shared_ptr<std::vector<std::shared_ptr<Location>>> validlocations_;
  };

} // namespace pegasus

#endif // PEGASUS_DATASET_DISTRIBUTOR_H