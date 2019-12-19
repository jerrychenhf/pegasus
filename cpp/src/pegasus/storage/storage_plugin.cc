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

#include <string>
#include <vector>

#include "pegasus/storage/storage_plugin.h"
#include "pegasus/util/consistent_hashing.h"

using namespace std;

namespace pegasus {

HDFSStoragePlugin::HDFSStoragePlugin() {

}

Status HDFSStoragePlugin::GetDataSet(std::string dataset_path, std::unique_ptr<DataSet>* dataset) {
  // Get table chunk from HDFS with parquet file path and row group ID.
  std::vector<std::string> file_list;
  return Status::OK();
}

} // namespace pegasus