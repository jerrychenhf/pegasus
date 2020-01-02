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

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#include "pegasus/common/server_options.h"

namespace pegasus {

ServerOptions::ServerOptions(std::string hostname, int32_t port,
    std::string storage_plugin_type) {

  pegasus::Location::ForGrpcTcp(hostname, port, &location_);

  if(storage_plugin_type == "HDFS") {
    storage_plugin_type_= StoragePlugin::StoragePluginType::HDFS;
  } else if(storage_plugin_type == "S3") {
    storage_plugin_type_= StoragePlugin::StoragePluginType::S3;
  } 
}

ServerOptions::ServerOptions(std::string hostname, int32_t port,
    std::string storage_plugin_type,
    std::string store_types) {

  ServerOptions(hostname, port, storage_plugin_type);
  std::vector<std::string> types;
  boost::split(types, store_types, boost::is_any_of(", "), boost::token_compress_on);
  for(std::vector<std::string>::iterator it = types.begin(); it != types.end(); ++it) {
    if(*it == "MEMORY") {
      store_types_.push_back(Store::StoreType::MEMORY);
    } else if(*it == "DCPMM") {
      store_types_.push_back(Store::StoreType::DCPMM);
    } else if(*it == "FILE") {
      store_types_.push_back(Store::StoreType::FILE);
    }
  }
}

}  // namespace pegasus