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

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "pegasus/common/status.h"
#include "arrow/util/uri.h"

using namespace std;

namespace pegasus {
/// \brief A host location (a URI)
class Location {
 public:
  /// \brief Initialize a blank location.
  Location();
  ~Location();
  /// \brief Initialize a location by parsing a URI string
  static Status Parse(const std::string& uri_string, Location* location);

  /// \brief Initialize a location for a non-TLS, gRPC-based Flight
  /// service from a host and port
  /// \param[in] host The hostname to connect to
  /// \param[in] port The port
  /// \param[out] location The resulting location
  static Status ForGrpcTcp(const std::string& host, const int port, Location* location);

  /// \brief Initialize a location for a TLS-enabled, gRPC-based Flight
  /// service from a host and port
  /// \param[in] host The hostname to connect to
  /// \param[in] port The port
  /// \param[out] location The resulting location
  static Status ForGrpcTls(const std::string& host, const int port, Location* location);

  /// \brief Initialize a location for a domain socket-based Flight
  /// service
  /// \param[in] path The path to the domain socket
  /// \param[out] location The resulting location
  static Status ForGrpcUnix(const std::string& path, Location* location);

  /// \brief Get a representation of this URI as a string.
  std::string ToString() const;

  /// \brief Get the scheme of this URI.
  std::string scheme() const;

  bool Equals(const Location& other) const;

  friend bool operator==(const Location& left, const Location& right) {
    return left.Equals(right);
  }
  friend bool operator!=(const Location& left, const Location& right) {
    return !(left == right);
  }
 private:
  std::shared_ptr<arrow::internal::Uri> uri_;

};

} // namespace pegasus