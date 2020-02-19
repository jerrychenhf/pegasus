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

/// \brief Data structure providing an identifier to use when requesting a table chunk
#pragma once

#include <string>
#include <vector>
#include <pegasus/common/status.h>

using namespace std;

namespace pegasus {

class Identity {
public:
  Identity() {}
  Identity(std::string dataset_path, std::string file_path, /*std::vector<int64_t> col_ids,*/ int64_t row_group_id, int64_t num_rows, int64_t bytes);
  
  std::string dataset_path() const;
  std::string file_path() const;
  std::vector<int> col_ids() const;
  int64_t row_group_id() const;
  int64_t num_rows() const;
  int64_t bytes() const;
  
  bool Equals(const Identity& other) const;

  friend bool operator==(const Identity& left, const Identity& right) {
    return left.Equals(right);
  }
  friend bool operator!=(const Identity& left, const Identity& right) {
    return !(left == right);
  }

  /// \brief Get the wire-format representation of this type.
  ///
  /// Useful when interoperating with non-Flight systems (e.g. REST
  /// services) that may want to return Flight types.
  Status SerializeToString(std::string* out) const;

  /// \brief Parse the wire-format representation of this type.
  ///
  /// Useful when interoperating with non-Flight systems (e.g. REST
  /// services) that may want to return Flight types.
  static Status Deserialize(const std::string& serialized, Identity* out);

 private:
  std::string dataset_path_;
  std::string file_path_;
  int64_t partid;
  std::vector<int> col_ids_;
  int64_t row_group_id_;
  int64_t num_rows_;
  int64_t bytes_;
};

} // namespace pegasus
