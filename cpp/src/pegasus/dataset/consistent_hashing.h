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

#ifndef PEGASUS_CONSISTENT_HASHING_H
#define PEGASUS_CONSISTENT_HASHING_H

#include <string>
#include <vector>
#include <boost/functional/hash.hpp>
#include <boost/format.hpp>
#include <boost/crc.hpp>
//#include <boost/algorithm/string/trim.hpp>

#include "pegasus/common/location.h"
#include "pegasus/dataset/identity.h"
#include "pegasus/dataset/dataset_distributor.h"
#include "pegasus/util/consistent_hash_map.hpp"

using namespace std;

namespace pegasus {
#define MAX_VIRT_NODE_NUM 100
#define MIN_VIRT_NODE_NUM 1

struct crc32_hasher {
    uint32_t operator()(const std::string& node) {
        boost::crc_32_type ret;
        ret.process_bytes(node.c_str(), node.size());
        return ret.checksum();
    }
    typedef uint32_t result_type;
};

typedef consistent_hash_map<std::string, crc32_hasher> consistent_hash_t;

  // Consistent hash ring to distribute items across nodes (locations). If we add 
  // or remove nodes, it minimizes the item migration.
  class ConsistentHashRing : DSDistributor {
  public:
    ConsistentHashRing();
    ~ConsistentHashRing();
    void PrepareValidLocations(std::shared_ptr<std::vector<Location>> locations, std::shared_ptr<std::vector<int64_t>> nodecacheMB);
    Status SetupDist();
    void AddLocation(unsigned int locidx);
    void AddLocation(Location location);
    void AddLocation(Location location, int num_virtual_nodes);
    void RemoveLocation(Location location);
    Location GetLocation(Identity identity);
    std::string GetHash(std::string key);
    void GetDistLocations(std::shared_ptr<std::vector<Identity>> vectident, std::shared_ptr<std::vector<Location>> vectloc);
    void GetDistLocations(std::shared_ptr<std::vector<Partition>> partitions);
  private:
//	  static struct conhash_s *conhash;
    consistent_hash_t consistent_hash_;
  };

} // namespace pegasus

#endif // PEGASUS_CONSISTENT_HASHING_H
