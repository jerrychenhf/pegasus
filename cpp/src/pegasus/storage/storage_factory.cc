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

#include "storage/storage_factory.h"

#include "arrow/util/uri.h"

namespace pegasus {

StorageFactory::StorageFactory() { }

StorageFactory::~StorageFactory() { }

Status StorageFactory::GetStorage(std::string url,
    std::shared_ptr<Storage>* storage) {

  Storage::StorageType storage_type;

  arrow::internal::Uri uri;
  uri.Parse(url);
  std::string host = uri.host();
  int32_t port = uri.port();
  if (uri.scheme() == "hdfs") {
    storage_type = Storage::HDFS;
  } else {
    return Status::Invalid("Invalid storage plugin type!");
  }

  switch (storage_type) {
    case Storage::HDFS:
      *storage = std::shared_ptr<Storage>(new HDFSStorage());
      RETURN_IF_ERROR(storage->get()->Init(host, port));
      RETURN_IF_ERROR(storage->get()->Connect());
      return Status::OK();
    default:
      return Status::Invalid("Invalid storage plugin type!");
  }
}

} // namespace pegasus