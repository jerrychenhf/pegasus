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
#include "common/location.h"
#include "dataset/identity.h"
#include "dataset/dataset_distributor.h"
#include "util/consistent_hash_map.hpp"

using namespace std;

namespace pegasus {
#define VIRT_NODE_DIVISOR 100 // place one virtual node for every 100MB cache
//#define MAX_VIRT_NODE_NUM 400 // the distribution is smoother with bigger value,
                                // replaced with FLAGS_max_virtual_node_num
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
  class ConsistentHashRing : PartitionDistributor {
  public:
    ConsistentHashRing();
    ~ConsistentHashRing();
    void PrepareValidLocations(std::shared_ptr<std::vector<Location>> locations,
      std::shared_ptr<std::vector<int64_t>> node_cache_capacity);
    Status SetupDistribution();

    void AddLocation(unsigned int index);
    void RemoveLocation(Location location);
    Location GetLocation(Identity identity);

    std::string GetHash(std::string key);

    void GetDistLocations(std::shared_ptr<std::vector<Identity>> identities,
      std::shared_ptr<std::vector<Location>> locations);
    void GetDistLocations(std::shared_ptr<std::vector<Partition>> partitions);
  private:
    consistent_hash_t consistent_hash_;
  };

struct ConHashMetrics {
  void Increment(std::string nodeaddr) {
    std::unordered_map<std::string, uint64_t>::iterator it = metrics_.find(nodeaddr);
    if (it == metrics_.end())
      metrics_[nodeaddr] = 1;
    else
      metrics_[nodeaddr]++;
  }

  void WriteToLog() {
    LOG(INFO) << "conhashmetrics distribution:";
    uint64_t totalcount = 0;
    for (auto& node : metrics_) {
      totalcount += node.second;
    }
    for (auto& node : metrics_) {
      LOG(INFO) << node.first << " : " << node.second << "/" << totalcount \
                << " (" << node.second*100/totalcount << "%)";
    }
  }

  void WriteAsJson() {
    // Not implemented yet
  }
  // Operation rates.
  std::unordered_map<std::string, uint64_t> metrics_;
};

} // namespace pegasus

#endif // PEGASUS_CONSISTENT_HASHING_H
