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

#include "pegasus/cache/memory_pool.h"


namespace pegasus {
    
DRAMMemoryPool::DRAMMemoryPool() { }

DRAMMemoryPool::~DRAMMemoryPool() { }

Status DRAMMemoryPool::Allocate(int64_t size, uint8_t** out) {
    *out = reinterpret_cast<uint8_t*>(std::malloc(size));
    return Status::OK();
  }

  void DRAMMemoryPool::Free(uint8_t* buffer, int64_t size)  { std::free(buffer); }

  Status DRAMMemoryPool::Reallocate(int64_t old_size, int64_t new_size, uint8_t** ptr) {
    *ptr = reinterpret_cast<uint8_t*>(std::realloc(*ptr, new_size));

    if (*ptr == NULL) {
      return Status::OutOfMemory("realloc of size ", new_size, " failed");
    }

    return Status::OK();
  }

  int64_t DRAMMemoryPool::bytes_allocated() const  { return -1; }

  std::string DRAMMemoryPool::backend_name() const { return "DRAM"; }

} // namespace pegasus