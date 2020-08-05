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

#ifndef PEGASUS_FILE_BATCH_READER_H
#define PEGASUS_FILE_BATCH_READER_H

#include "arrow/status.h"
#include "arrow/ipc/message.h"
#include "rpc/file_batch.h"

using namespace std;

namespace pegasus {

class CachedColumn;

namespace rpc {

class FileBatchReader {
 public:
  FileBatchReader() {};

  virtual ~FileBatchReader() = default;
  
  /// \return the shared schema of the file batches in the stream
  virtual std::shared_ptr<arrow::Schema> schema() const = 0;
  	
  /// \brief Read the next file batch in the stream. Return null for batch
  /// when reaching end of stream
  ///
  /// \param[out] batch the next loaded batch, null at end of stream
  /// \return Status
  virtual arrow::Status ReadNext(std::shared_ptr<FileBatch>* batch) = 0;
 
};

/// \brief Compute a stream of record batches from a (possibly chunked) Table
///
/// The conversion is zero-copy: each file batch is a view of row group columns.
class  CachedFileBatchReader : public FileBatchReader {
 public:
  /// \brief Construct a CachedFileBatchReader for the given CachedColumns
  explicit CachedFileBatchReader(std::vector<std::shared_ptr<CachedColumn>> columns, std::shared_ptr<arrow::Schema> schema);
  CachedFileBatchReader();

  std::shared_ptr<arrow::Schema> schema() const override;

  arrow::Status ReadNext(std::shared_ptr<FileBatch>* out) override;

 private:
  std::vector<std::shared_ptr<CachedColumn>> columns_;
  int rowgroup_nums_;
  int absolute_rowgroup_position_;
  std::shared_ptr<arrow::Schema> schema_;
};

class  FileBatchStreamReader : public FileBatchReader {
 public:
  static arrow::Status Open(std::unique_ptr<arrow::ipc::MessageReader> message_reader,
                     std::unique_ptr<FileBatchReader>* out); 
  
};

} // namespace rpc

} // namespace pegasus

#endif  // PEGASUS_FILE_BATCH_READER_H
