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

#ifndef PEGASUS_PARQUET_METADATA_H
#define PEGASUS_PARQUET_METADATA_H

#include "parquet/api/reader.h"

#include "pegasus/common/status.h"

namespace pegasus {

using FileMetaData = parquet::FileMetaData;

class ParquetMetadata {

 public:
  ParquetMetadata(std::shared_ptr<std::vector<std::string>> flie_path_list);
  Status GetFilesMeta(std::shared_ptr<std::vector<std::shared_ptr<FileMetaData>>>* files_metadata);

 private:
  std::shared_ptr<std::vector<std::string>> flie_path_list_;
  std::vector<FileMetaData> files_metadata_;
};

} // namespace pegasus

#endif  // PEGASUS_PARQUET_METADATA_H
