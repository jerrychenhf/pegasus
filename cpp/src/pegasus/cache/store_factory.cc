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

#include "pegasus/cache/store_factory.h"


namespace pegasus {
    
StoreFactory::StoreFactory() { }

StoreFactory::~StoreFactory() { }

Status StoreFactory::GetStore(Store::StoreType cache_type, std::shared_ptr<Store>* store) {
  if (cache_type == Store::StoreType::MEMORY) {
    *store = std::shared_ptr<MemoryStore>(new MemoryStore());
    return Status::OK();
  } else if (cache_type == Store::StoreType::DCPMM) {
    *store = std::shared_ptr<DCPMMStore>(new DCPMMStore());
    return Status::OK();
  } else if (cache_type == Store::StoreType::FILE) {
    *store = std::shared_ptr<FileStore>(new FileStore());
    return Status::OK();
  } else {
    return Status::Invalid("Invalid store type!");
  }

}

} // namespace pegasus