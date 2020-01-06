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

#ifndef PEGASUS_DATASET_H
#define PEGASUS_DATASET_H

#include <string>
#include <vector>

#include "pegasus/dataset/partition.h"
#include "pegasus/util/visibility.h"

namespace pegasus {

/// \brief The access coordinates for retireval of a dataset
class PEGASUS_EXPORT DataSet {
 public:
  struct Data {
    std::string schema;
    /// Path identifying a particular dataset. 
    std::string dataset_path;
    std::vector<Partition> endpoints;
    uint64_t timestamp;
    int64_t total_records;
    int64_t total_bytes;
  };

  explicit DataSet(const Data& data) : data_(data) {}
  explicit DataSet(Data&& data)
      : data_(std::move(data)) {}

  /// The path of the dataset
  const std::string& dataset_path() const { return data_.dataset_path; }

  /// A list of endpoints associated with the dataset.
  const std::vector<Partition>& endpoints() const { return data_.endpoints; }

  /// The total number of records (rows) in the dataset. If unknown, set to -1
  int64_t total_records() const { return data_.total_records; }

  /// The total number of bytes in the dataset. If unknown, set to -1
  int64_t total_bytes() const { return data_.total_bytes; }

 private:
  Data data_;
};

} // namespace pegasus

#endif  // PEGASUS_DATASET_H