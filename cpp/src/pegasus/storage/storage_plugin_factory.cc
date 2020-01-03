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

#include "pegasus/storage/storage_plugin_factory.h"

namespace pegasus {

StoragePluginFactory::StoragePluginFactory() { }

StoragePluginFactory::~StoragePluginFactory() { }

Status StoragePluginFactory::GetStoragePlugin(StoragePlugin::StoragePluginType storage_plugin_type,
  std::shared_ptr<StoragePlugin>* storage_plugin) {

  switch (storage_plugin_type) {
    case StoragePlugin::HDFS:
      *storage_plugin = std::shared_ptr<StoragePlugin>(new HDFSStoragePlugin());
      return Status::OK();
    default:
      return Status::Invalid("Invalid storage plugin type!");
  }
}

} // namespace pegasus