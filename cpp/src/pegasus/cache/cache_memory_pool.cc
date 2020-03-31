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

#include "cache/cache_memory_pool.h"
#include "common/logging.h"

namespace pegasus {

constexpr size_t kAlignment = 64;

// A static piece of memory for 0-size allocations, so as to return
// an aligned non-null pointer.
alignas(kAlignment) static uint8_t zero_size_area[1];

CacheMemoryPool::CacheMemoryPool(std::shared_ptr<CacheEngine> cache_engine)
  : cache_engine_(cache_engine) {
}
CacheMemoryPool::~CacheMemoryPool() {}

Status CacheMemoryPool::Create() {  
  // TO BE IMPPROVED
  // for choose different stores from engine according to the dataset
  RETURN_IF_ERROR(cache_engine_->GetCacheStore(&cache_store_));
  return Status::OK();
}

Status CacheMemoryPool::GetCacheRegion(int64_t size, StoreRegion* store_region) {
  if (cache_store_ == nullptr)
    return Status::Invalid("Cache store is not correctly initialized.");
  
  RETURN_IF_ERROR(cache_store_->Allocate(size, store_region));
  return Status::OK();
}

arrow::Status CacheMemoryPool::Allocate(int64_t size, uint8_t** out) {
  if (size < 0) {
    return arrow::Status::Invalid("negative malloc size");
  }

  if (size == 0) {
    *out = zero_size_area;
    return arrow::Status::OK();
  }
  
  StoreRegion store_region;
  Status status = GetCacheRegion(size, &store_region);
  if(!status.ok()) {
    return arrow::Status::OutOfMemory("Failed to allocate cache region in cache memory pool");
  }
  
  *out = store_region.address();

  stats_.UpdateAllocatedBytes(size);
  LOG(INFO) << "Allocate memory in cache memory pool and the allocated size is " << size;
  return arrow::Status::OK();
}

void CacheMemoryPool::Free(uint8_t* buffer, int64_t size)  {

  if (cache_store_ == nullptr)
    return;
  
  if (buffer == zero_size_area) {
    DCHECK_EQ(size, 0);
    return;
  }
    
  StoreRegion storeRegion(buffer, size, size);
  cache_store_->Free(&storeRegion);
  LOG(INFO) << "Free memory in cache memory pool and the free size is " << size;
 
  stats_.UpdateAllocatedBytes(-size);
}

arrow::Status CacheMemoryPool::Reallocate(int64_t old_size, int64_t new_size, uint8_t** ptr) {
  if (new_size < 0) {
    return arrow::Status::Invalid("negative realloc size");
  }

  if (static_cast<uint64_t>(new_size) >= std::numeric_limits<size_t>::max()) {
    return arrow::Status::CapacityError("realloc overflows size_t");
  }

  uint8_t* previous_ptr = *ptr;

  if (previous_ptr == zero_size_area) {
    DCHECK_EQ(old_size, 0);
    return Allocate(new_size, ptr);
  }

  if (new_size == 0) {
    Free(previous_ptr, old_size);
    *ptr = zero_size_area;
    return arrow::Status::OK();
  }

  if (cache_store_ == nullptr)
    return arrow::Status::Invalid("Cache store is not correctly initialized.");


  StoreRegion store_region;
  store_region.reset_address(*ptr, old_size, old_size);

  Status status = cache_store_->Reallocate(old_size, new_size, &store_region);

  if(!status.ok()) {
    *ptr = previous_ptr;
    return arrow::Status::OutOfMemory("Failed to reallocate memory in cache memory pool");
  }

  *ptr = store_region.address();

  stats_.UpdateAllocatedBytes(new_size - old_size);
  LOG(INFO) << "Reallocate memory in cache memory pool and the reallocate new size is "
   << new_size << " and the old size is " << old_size;
  return arrow::Status::OK();
}

int64_t CacheMemoryPool::bytes_allocated() const  {
  return stats_.bytes_allocated();;
}
  
int64_t CacheMemoryPool::max_memory() const {
  return stats_.max_memory();
}

std::string CacheMemoryPool::backend_name() const {
  if (cache_store_ != nullptr) {
    return cache_store_->GetStore()->GetStoreName();
  } else {
    return "MEMORY";
  } 
}

} // namespace pegasus