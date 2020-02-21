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

#include "util/slice.h"

#include <cctype>

#include "gutil/port.h"
#include "gutil/stringprintf.h"
#include "common/status.h"
#include "common/logging.h"

namespace pegasus {

Status Slice::check_size(size_t expected_size) const {
  if (PREDICT_FALSE(size() != expected_size)) {
    return Status::Invalid(StringPrintf("Unexpected Slice size. "
        "Expected %zu but got %zu.", expected_size, size()));
  }
  return Status::OK();
}

// Return a string that contains the copy of the referenced data.
std::string Slice::ToString() const {
  return std::string(reinterpret_cast<const char *>(data_), size_);
}

std::string Slice::ToDebugString(size_t max_len) const {
  size_t bytes_to_print = size_;
  bool abbreviated = false;
  if (max_len != 0 && bytes_to_print > max_len) {
    bytes_to_print = max_len;
    abbreviated = true;
  }

  int size = 0;
  for (int i = 0; i < bytes_to_print; i++) {
    if (!isgraph(data_[i])) {
      size += 4;
    } else {
      size++;
    }
  }
  if (abbreviated) {
    size += 20;  // extra padding
  }

  std::string ret;
  ret.reserve(size);
  for (int i = 0; i < bytes_to_print; i++) {
    if (!isgraph(data_[i])) {
      StringAppendF(&ret, "\\x%02x", data_[i] & 0xff);
    } else {
      ret.push_back(data_[i]);
    }
  }
  if (abbreviated) {
    StringAppendF(&ret, "...<%zd bytes total>", size_);
  }
  return ret;
}

bool IsAllZeros(const Slice& s) {
  // Walk a pointer through the slice instead of using s[i]
  // since this is way faster in debug mode builds. We also do some
  // manual unrolling for the same purpose.
  const uint8_t* p = &s[0];
  int rem = s.size();

  while (rem >= 8) {
    if (UNALIGNED_LOAD64(p) != 0) return false;
    rem -= 8;
    p += 8;
  }

  while (rem > 0) {
    if (*p++ != '\0') return false;
    rem--;
  }
  return true;
}

}  // namespace pegasus
