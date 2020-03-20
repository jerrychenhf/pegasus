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

#ifndef PEGASUS_DATASET_REQUEST_H_
#define PEGASUS_DATASET_REQUEST_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <arrow/type.h>
#include <boost/algorithm/string.hpp>

#include "dataset/filter.h"

using namespace std;

namespace pegasus {

class DataSetRequest {

 public:
  DataSetRequest();
  ~DataSetRequest();

  typedef std::unordered_map<std::string, std::string> RequestProperties;

  void set_dataset_path(const std::string& dataset_path);
  void set_properties(const RequestProperties& properties);
  void set_column_indices(const std::vector<int32_t>& column_indices);

  const std::string& get_dataset_path();
  const RequestProperties& get_properties();
  const std::string& get_format();
  const std::vector<std::string>& get_column_names();
  const std::vector<int32_t>& get_column_indices();
  const std::vector<Filter>& get_filters();

  static const std::string CATALOG_PROVIDER;
  static const std::string FILE_FORMAT;
  static const std::string TABLE_LOCATION;
  static const std::string COLUMN_NAMES;

  private:
   std::string dataset_path_;
   RequestProperties properties_;

   std::string format_;
   std::vector<std::string> column_names_;
   std::vector<Filter> filters_;
   std::vector<std::int32_t> column_indices_;
   std::shared_ptr<arrow::Schema> schema_;
};


} // namespace pegasus

#endif  // PEGASUS_DATASET_REQUEST_H_
