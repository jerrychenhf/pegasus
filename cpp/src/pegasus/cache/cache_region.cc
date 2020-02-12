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

/// \brief Data structure providing an opaque identifier or credential to use
/// when requesting a data stream with the DoGet RPC
#include "cache/cache_region.h"

namespace pegasus {

CacheRegion::CacheRegion(){}

CacheRegion::CacheRegion(uint8_t* base_offset, long length, long occupied_size,
 arrow::ChunkedArray* chunked_array): address_(base_offset), length_(length),
  occupied_size_(occupied_size), chunked_array_(chunked_array) {}

 CacheRegion::~CacheRegion () {}  
 uint8_t* CacheRegion::address() const {
   return address_;
 }

 long CacheRegion::length() const {
    return length_;
 }
 long CacheRegion::occupies_size() const {
    return occupied_size_;
 }

 arrow::ChunkedArray* CacheRegion::chunked_array() const {
    return chunked_array_;
 }

} // namespace pegasus
