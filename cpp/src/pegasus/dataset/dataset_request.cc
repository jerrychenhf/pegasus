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
  const std::string DataSetRequest::CATALOG_PROVIDER = "catalog.provider";
  const std::string DataSetRequest::FILE_FORMAT = "file.format";
  const std::string DataSetRequest::TABLE_LOCATION = "table.location";
  const std::string DataSetRequest::COLUMN_NAMES = "column.names";

  DataSetRequest::DataSetRequest() {
  }

  DataSetRequest::~DataSetRequest() {
  }

  void DataSetRequest::set_dataset_path(const std::string& dataset_path) {
    dataset_path_ = dataset_path;
  }

  void DataSetRequest::set_properties(
    const DataSetRequest::RequestProperties& properties) {
    properties_ = properties;
  }

  const std::string& DataSetRequest::get_dataset_path() {
    return dataset_path_;
  }

  const DataSetRequest::RequestProperties& DataSetRequest::get_properties() {
    return properties_;
  }

  const std::vector<std::string>& DataSetRequest::get_column_names() {
    std::unordered_map<std::string, std::string>::const_iterator it = 
        properties_.find(DataSetRequest::COLUMN_NAMES);
    if (it != properties_.end()) {
      std::string column_name_string = it->second;
      boost::split(column_names_, column_name_string,
          boost::is_any_of(","), boost::token_compress_on);
      for(int i = 0; i < column_names_.size(); i++) {
        boost::trim(column_names_[i]);
      }
    }
    return column_names_;
  }

  const std::vector<Filter>& DataSetRequest::get_filters() {
    return filters_;
  }

  void DataSetRequest::set_column_indices(const std::vector<int32_t>& column_indices) {
    column_indices_ = column_indices;
  }

  const std::vector<int32_t>& DataSetRequest::get_column_indices() {
    return column_indices_;
  }

} // namespace pegasus
