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

#ifndef PEGASUS_OBJECT_ID_H
#define PEGASUS_OBJECT_ID_H

namespace pegasus {

constexpr int64_t kObjectIDSize = 20;

class ObjectID {
 public:
  static ObjectID from_binary(const std::string& binary);
  bool operator==(const ObjectID& rhs) const;
  const uint8_t* data() const;
  uint8_t* mutable_data();
  std::string binary() const;
  std::string hex() const;
  size_t hash() const;
  static int64_t size() { return kObjectIDSize; }

 private:
  uint8_t id_[kObjectIDSize];
};
} // namespace pegasus

#endif  // PEGASUS_DATASET_CACHE_BLOCK_MANAGER_H
