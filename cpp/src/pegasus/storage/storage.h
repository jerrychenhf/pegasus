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

#ifndef PEGASUS_STORAGE_H
#define PEGASUS_STORAGE_H

#include "arrow/io/hdfs.h"
#include "common/status.h"
#include "dataset/dataset.h"

namespace pegasus {

class Storage {
 public:
  virtual Status Init(std::string host, int32_t port) = 0;
  virtual Status Connect() = 0;
  virtual Status GetModifedTime(std::string dataset_path, uint64_t* modified_time) = 0;
  virtual Status ListFiles(std::string dataset_path, std::vector<std::string>* file_list,
                           int64_t* total_bytes) = 0;
    
  enum StorageType {
    UNKNOWN,
    HDFS,
    S3
  };

  virtual StorageType GetStorageType() = 0;

 private:
  StorageType storage_type_;
};

} // namespace pegasus

#endif  // PEGASUS_STORAGE_H