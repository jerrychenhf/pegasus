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

#include "storage/file_status.h"

namespace pegasus
{
  void FileStatus::set_file_path(std::string file_path) {
      file_path_ = file_path;
  }
  
  void FileStatus::set_size(int64_t size_in_bytes) {
      size_in_bytes_ = size_in_bytes;
  }

  std::string FileStatus::get_file_path() {
      return file_path_;
  }

  int64_t FileStatus::get_size() {
      return size_in_bytes_;
  }

} // namespace pegasus
