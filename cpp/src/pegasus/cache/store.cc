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

#include "pegasus/cache/store.h"

namespace pegasus {

MemoryStore::MemoryStore() {

}

Status MemoryStore::Allocate(long size, std::shared_ptr<CacheRegion>* cache_region) {

}

Status MemoryStore::GetTotalSize(long& total_size) {
    
}

Status MemoryStore::GetUsedSize(long& used_size) {
    
}

std::string MemoryStore::GetStoreName() {
    return "MEMORY";
}

DCPMMStore::DCPMMStore() {

}

Status DCPMMStore::Allocate(long size, std::shared_ptr<CacheRegion>* cache_region) {

}

Status DCPMMStore::GetTotalSize(long& total_size) {
    
}

Status DCPMMStore::GetUsedSize(long& used_size) {
    
}

std::string DCPMMStore::GetStoreName() {
    return "DCPMM";
}

// FileStore::FileStore() {

// }

// Status FileStore::Allocate(long size) {

// }

// Status FileStore::GetTotalSize(long& total_size) {
    
// }

// Status FileStore::GetUsedSize(long& used_size) {

// }

// std::string FileStore::GetStoreName() {
//     return "FILE";
// }

} // namespace pegasus