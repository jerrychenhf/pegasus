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

#include "dataset/request_identity.h"

namespace pegasus {

RequestIdentity::RequestIdentity(){}

RequestIdentity::RequestIdentity(std::string dataset_path,
 std::string partition_path, std::vector<int> column_indices)
    : dataset_path_(dataset_path), partition_path_(partition_path), column_indices_(column_indices) {}

const std::string& RequestIdentity::dataset_path() {
  return dataset_path_;
}

const std::string& RequestIdentity::partition_path() {
  return partition_path_;
}

const std::vector<int>& RequestIdentity::column_indices(){
  return column_indices_;
}

void RequestIdentity::set_schema(shared_ptr<arrow::Schema> schema) {
  schema_ = schema;
}

void RequestIdentity::get_schema(std::shared_ptr<arrow::Schema>* schema) {
  *schema = schema_;
}

} // namespace pegasus
