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

#include "storage/storage_plugin_factory.h"

#include "arrow/util/uri.h"

namespace pegasus {

StoragePluginFactory::StoragePluginFactory() { }

StoragePluginFactory::~StoragePluginFactory() { }

Status StoragePluginFactory::GetStoragePlugin(std::string url,
    std::shared_ptr<StoragePlugin>* storage_plugin) {

  StoragePlugin::StoragePluginType storage_plugin_type;

  arrow::internal::Uri uri;
  uri.Parse(url);
  std::string host = uri.host();
  int32_t port = uri.port();
  // if (url.find("hdfs://") != std::string::npos) {
  if (uri.scheme() == "hdfs") {
    storage_plugin_type = StoragePlugin::HDFS;
  } else {
    return Status::Invalid("Invalid storage plugin type!");
  }

  switch (storage_plugin_type) {
    case StoragePlugin::HDFS:
      *storage_plugin = std::shared_ptr<StoragePlugin>(new HDFSStoragePlugin());
      RETURN_IF_ERROR(storage_plugin->get()->Init(host, port));
      RETURN_IF_ERROR(storage_plugin->get()->Connect());
      return Status::OK();
    default:
      return Status::Invalid("Invalid storage plugin type!");
  }
}

} // namespace pegasus