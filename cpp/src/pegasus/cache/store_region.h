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

#ifndef PEGASUS_STORE_REGION_H
#define PEGASUS_STORE_REGION_H

#include <string>

using namespace std;

namespace pegasus {

class StoreRegion {
 public:
  StoreRegion();
  StoreRegion(uint8_t* address, long length, long occupied_size);
  
  ~StoreRegion();
  
  void reset_address(uint8_t* address,
   int64_t length, int64_t occupied_size) {
    address_ = address;
    length_ = length;
    occupied_size_ = occupied_size;
  }
  
  uint8_t* address() const;
  int64_t length() const;
  int64_t occupies_size() const;

 private:
  uint8_t* address_;
  int64_t length_;
  int64_t occupied_size_;
};

} // namespace pegasus

#endif  // PEGASUS_STORE_REGION_H