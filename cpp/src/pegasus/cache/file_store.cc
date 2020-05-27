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

#include "cache/file_store.h"
#include "ipc/allocator.h"
#include "cache/store_manager.h"

DECLARE_int32(store_file_capacity_gb);

namespace pegasus {
constexpr int64_t kBlockSize = 64;

FileStore::FileStore(int64_t capacity)
  : capacity_(capacity),
    used_size_(0) {
}

Status FileStore::Init(const std::unordered_map<string, string>* properties) {
  LOG(INFO) << "Init the binary store";
  // Get the file store path from properties
  auto entry  = properties->find(StoreManager::STORE_PROPERTY_PATH);
  
  std::string file_store_path = "";

  if (entry == properties->end()) {
    file_store_path = "/dev/shm";
  } else {
    file_store_path = entry->second;
  }
  LOG(INFO) << "The file store path is " << file_store_path;

  int64_t capacity = ((int64_t) FLAGS_store_file_capacity_gb) * StoreManager::GIGABYTE;

  Allocator::SetFootprintLimit(static_cast<size_t>(capacity));
  
  void* pointer = Allocator::Memalign(
        kBlockSize, Allocator::GetFootprintLimit() - 256 * sizeof(size_t));
  DCHECK(pointer != nullptr);
    
  Allocator::Free(pointer, Allocator::GetFootprintLimit() - 256 * sizeof(size_t));
  return Status::OK();
}

Status FileStore::Allocate(int64_t size, StoreRegion* store_region) {

  return Status::OK();
}

Status FileStore::Free(StoreRegion* store_region) {

  return Status::OK();
}

int64_t FileStore::GetFreeSize() {
  return capacity_ - used_size_; 
}

int64_t FileStore::GetUsedSize() {
  return used_size_;
}

std::string FileStore::GetStoreName() {
    return "BINARY";
}

} // namespace pegasus