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

#ifndef PEGASUS_DATASET_REQUEST_H_
#define PEGASUS_DATASET_REQUEST_H_

#include "dataset/identity.h"

namespace pegasus {
  class RequestIdentity {

 public:
  RequestIdentity();
  RequestIdentity(std::string dataset_path, std::string partition_path, std::vector<int> column_indices);


  const std::string& dataset_path();
  const std::vector<int>& column_indices();
  const std::string& partition_path();

  private:
   std::string dataset_path_;
   std::string partition_path_;
   std::vector<int> column_indices_;
};

} // namespace pegasus
#endif  // PEGASUS_DATASET_REQUEST_H_
