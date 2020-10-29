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

/// \brief Data structure providing an opaque identifier or credential to use
/// when requesting a data stream with the DoGet RPC

#include "dataset/identity.h"

namespace pegasus {

Identity::Identity(std::string dataset_path, std::string partition_id)
    : dataset_path_(dataset_path), partition_id_(partition_id) {}
  
std::string Identity::dataset_path() const {
    return dataset_path_;
  }

std::string Identity::partition_id() const {
    return partition_id_;
  }

bool Identity::Equals(const Identity& other) const {
	return false;
}

Status Identity::SerializeToString(std::string* out) const {
//	out->assign(partition_id_ + std::to_string(row_group_id_));
	out->assign(partition_id_);
	return Status::OK();
}

} // namespace pegasus
