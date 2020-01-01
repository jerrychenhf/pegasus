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

#include "pegasus/util/uri.h"

#include <cstring>
#include <sstream>
#include <vector>

namespace pegasus {
namespace internal {

Uri::Uri() {}

Uri::~Uri() {}

std::string Uri::scheme() const {}

std::string Uri::host() const {}

bool Uri::has_host() const {}

std::string Uri::port_text() const {}

int32_t Uri::port() const {}

std::string Uri::path() const {}

std::string Uri::query_string() const {}

const std::string& Uri::ToString() const {}

Status Uri::Parse(const std::string& uri_string) {}

}  // namespace internal
}  // namespace pegasus
