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

#ifndef PEGASUS_SERVER_OPTIONS_H
#define PEGASUS_SERVER_OPTIONS_H

#include "pegasus/common/location.h"
#include "pegasus/cache/store.h"
#include "pegasus/storage/storage_plugin.h"

namespace pegasus {

class ServerOptions {
 public:
  ServerOptions(const Location& location,
      const StoragePlugin::StoragePluginType& storage_plugin_type);
  
  ServerOptions(const Location& location,
      const StoragePlugin::StoragePluginType& storage_plugin_type,
      const Store::StoreType& store_types);

  ~ServerOptions();

  /// \brief The host & port (or domain socket path) to listen on.
  /// Use port 0 to bind to an available port.
  Location location_;
  StoragePlugin::StoragePluginType storage_plugin_type_;
  std::vector<Store::StoreType> store_types_;

};

}  // namespace pegasus

#endif  // PEGASUS_SERVER_OPTIONS_H