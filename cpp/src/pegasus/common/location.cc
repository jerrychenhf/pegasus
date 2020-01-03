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

#include <memory>
#include <sstream>
#include <utility>

#include "pegasus/common/location.h"
#include "arrow/status.h"

namespace pegasus {

Location::Location() { uri_ = std::make_shared<arrow::internal::Uri>(); }

//TODO: is it correct to assign uri_??
Location::Location(const Location &loc) {
  uri_ = std::make_shared<arrow::internal::Uri>();
  uri_->Parse(loc.uri_->ToString());
  cachesizegb_ = loc.cachesizegb_;
}

Location::~Location() {
  
}

Status Location::Parse(const std::string& uri_string, Location* location) {
  arrow::Status st = location->uri_->Parse(uri_string);
  if (!st.ok()) {
      return Status(StatusCode(st.code()), st.message());
  }
  return Status::OK();
}

Status Location::ForGrpcTcp(const std::string& host, const int port, Location* location) {
  std::stringstream uri_string;
  uri_string << "grpc+tcp://" << host << ':' << port;
  return Location::Parse(uri_string.str(), location);
}

Status Location::ForGrpcTls(const std::string& host, const int port, Location* location) {

}

Status Location::ForGrpcUnix(const std::string& path, Location* location) {

}

std::string Location::ToString() const { return uri_->ToString(); }
std::string Location::scheme() const {
  std::string scheme = uri_->scheme();
  if (scheme.empty()) {
    // Default to grpc+tcp
    return "grpc+tcp";
  }
  return scheme;
}

bool Location::Equals(const Location& other) const {
  return ToString() == other.ToString();
}

} // namespace pegasus
