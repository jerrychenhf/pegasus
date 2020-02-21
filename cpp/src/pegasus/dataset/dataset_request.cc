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

#include "dataset/dataset_request.h"

namespace pegasus {

  DataSetRequest::DataSetRequest() {


  }
  DataSetRequest::~DataSetRequest() {

  }
    
  void DataSetRequest::set_dataset_path(const std::string& dataset_path) {
    dataset_path_ = dataset_path;
  }

  void DataSetRequest::set_options(const DataSetRequest::RequestProperties& options) {
    options_ = options;
  }

  const std::string& DataSetRequest::get_dataset_path() {
    return dataset_path_;
  }

  const DataSetRequest::RequestProperties& DataSetRequest::get_options() {
    return options_;
  }

  const std::string& DataSetRequest::get_format() {
    return format_;
  }

  const std::vector<uint32_t>& DataSetRequest::get_column_indices() {
    return column_indices_;
  }

  const std::vector<Filter>& DataSetRequest::get_filters() {
    return filters_;
  }

} // namespace pegasus
