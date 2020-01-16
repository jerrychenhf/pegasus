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

#ifndef PEGASUS_STORAGE_PLUGIN_FACTORY_H
#define PEGASUS_STORAGE_PLUGIN_FACTORY_H

#include <string>
#include "pegasus/storage/storage_plugin.h"

namespace pegasus {
class StoragePluginFactory {
 public:
  StoragePluginFactory();
  ~StoragePluginFactory();
    
  Status GetStoragePlugin(std::string url, std::shared_ptr<StoragePlugin>* storage_plugin);
};

} // namespace pegasus

#endif  // PEGASUS_STORAGE_PLUGIN_FACTORY_H