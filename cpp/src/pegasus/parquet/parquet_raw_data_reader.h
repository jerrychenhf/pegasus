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

#ifndef PEGASUS_PARQUET_RAW_DATA_READER_H
#define PEGASUS_PARQUET_RAW_DATA_READER_H

#include "parquet/file_reader.h"
#include "arrow/io/interfaces.h"
#include "parquet/properties.h"
#include "arrow/buffer.h"
#include "arrow/table.h"

#include "common/status.h"

namespace pegasus {

class ParquetRawDataReader {
 public:
  ParquetRawDataReader(const std::shared_ptr<arrow::io::RandomAccessFile>& file,
                       const parquet::ReaderProperties& properties);

  Status GetColumnBuffer(int row_group_index, int column_index,
                         std::shared_ptr<arrow::Buffer>* buffer);

  Status GetRowGroupReader(int i, std::shared_ptr<parquet::RowGroupReader>* row_group_reader);
  Status GetColumnReader(int i, std::shared_ptr<parquet::ColumnReader>* column_reader);
  int RowGroupsNum() const;

 private:
  std::unique_ptr<parquet::ParquetFileReader> file_reader_;
};

} // namespace pegasus

#endif  // PEGASUS_PARQUET_RAW_DATA_READER_H