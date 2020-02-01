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
#include "pegasus/dataset/identity.h"

namespace pegasus {

Identity::Identity(std::string dataset_path, std::string file_path, /*std::vector<int64_t> col_ids,*/ int64_t row_group_id, int64_t num_rows, int64_t bytes)
    : dataset_path_(dataset_path), file_path_(file_path), /*col_ids_(col_ids),*/ row_group_id_(row_group_id), num_rows_(num_rows), bytes_(bytes) {}
  
std::string Identity::dataset_path() const {
    return dataset_path_;
  }

std::string Identity::file_path() const {
    return file_path_;
  }

std::vector<int64_t> Identity::col_ids() const { 
    return col_ids_;
  }

int64_t Identity::row_group_id() const { 
    return row_group_id_;
  }

int64_t Identity::num_rows() const {
    return num_rows_;
  }

int64_t Identity::bytes() const {
    return bytes_;
  }
  
bool Identity::Equals(const Identity& other) const {
	return false;
}

Status Identity::SerializeToString(std::string* out) const {
//	out->assign(file_path_ + std::to_string(row_group_id_));
	out->assign(file_path_);
	return Status::OK();
}

} // namespace pegasus
