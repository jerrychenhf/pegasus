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

#include "dataset/filter.h"

using namespace std;

namespace pegasus {

class DataSetRequest {

 public:
  DataSetRequest();
  ~DataSetRequest();
  
  typedef std::unordered_map<std::string, std::string> RequestOptions;

  void set_dataset_path(std::string dataset_path);
  void set_options(RequestOptions *options);

  std::string get_dataset_path();
  RequestOptions* get_options();
  std::string get_format();
  std::vector<uint32_t>* get_column_indices();
  std::vector<Filter>* get_filters();

  private:
   std::string dataset_path_;
   RequestOptions* options_;

   std::string format_;
   std::vector<uint32_t>* column_indices_;
   std::vector<Filter>* filters_;
};


} // namespace pegasus

#endif  // PEGASUS_DATASET_REQUEST_H_
