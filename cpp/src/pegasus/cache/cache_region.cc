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
#include "pegasus/cache/cache_region.h"

namespace pegasus {

CacheRegion::CacheRegion(){}

CacheRegion::CacheRegion(long base_offset, long length, long occupied_size)
    : base_offset_(base_offset), length_(length), occupied_size_(occupied_size) {}

 CacheRegion::~CacheRegion () {}  

 long CacheRegion::length() {
     return length_;
 }
 long CacheRegion::occupies_size() {
     return occupied_size_;
 }

} // namespace pegasus
