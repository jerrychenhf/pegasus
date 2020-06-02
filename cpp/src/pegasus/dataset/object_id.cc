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

#include "dataset/object_id.h"

namespace pegasus {

ObjectID ObjectID::from_binary(const std::string& binary) {
  ObjectID id;
  std::memcpy(&id, binary.data(), sizeof(id));
  return id;
}

const uint8_t* ObjectID::data() const { return id_; }

uint8_t* ObjectID::mutable_data() { return id_; }

std::string ObjectID::binary() const {
  return std::string(reinterpret_cast<const char*>(id_), kObjectIDSize);
}

std::string ObjectID::hex() const {
  constexpr char hex[] = "0123456789abcdef";
  std::string result;
  for (int i = 0; i < kObjectIDSize; i++) {
    unsigned int val = id_[i];
    result.push_back(hex[val >> 4]);
    result.push_back(hex[val & 0xf]);
  }
  return result;
}

// This code is from https://sites.google.com/site/murmurhash/
// and is public domain.
uint64_t MurmurHash64A(const void* key, int len, unsigned int seed) {
  const uint64_t m = 0xc6a4a7935bd1e995;
  const int r = 47;

  uint64_t h = seed ^ (len * m);

  const uint64_t* data = reinterpret_cast<const uint64_t*>(key);
  const uint64_t* end = data + (len / 8);

  while (data != end) {
    uint64_t k = arrow::util::SafeLoad(data++);

    k *= m;
    k ^= k >> r;
    k *= m;

    h ^= k;
    h *= m;
  }

  const unsigned char* data2 = reinterpret_cast<const unsigned char*>(data);

  switch (len & 7) {
    case 7:
      h ^= uint64_t(data2[6]) << 48;  // fall through
    case 6:
      h ^= uint64_t(data2[5]) << 40;  // fall through
    case 5:
      h ^= uint64_t(data2[4]) << 32;  // fall through
    case 4:
      h ^= uint64_t(data2[3]) << 24;  // fall through
    case 3:
      h ^= uint64_t(data2[2]) << 16;  // fall through
    case 2:
      h ^= uint64_t(data2[1]) << 8;  // fall through
    case 1:
      h ^= uint64_t(data2[0]);
      h *= m;
  }

  h ^= h >> r;
  h *= m;
  h ^= h >> r;

  return h;
}

size_t ObjectID::hash() const { return MurmurHash64A(&id_[0], kObjectIDSize, 0); }

bool ObjectID::operator==(const ObjectID& rhs) const {
  return std::memcmp(data(), rhs.data(), kObjectIDSize) == 0;
}
} // namespace pegasus
