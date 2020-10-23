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

#include "parquet/parquet_raw_data_reader.h"

namespace pegasus {

ParquetRawDataReader::ParquetRawDataReader(
  const std::shared_ptr<arrow::io::RandomAccessFile>& file,
  const parquet::ReaderProperties& properties) {
  file_reader_ = parquet::ParquetFileReader::Open(file, properties);
}

int ParquetRawDataReader::RowGroupsNum() const {
  return file_reader_->metadata()->num_row_groups();
}

Status ParquetRawDataReader::GetColumnBuffer(
  int row_group_index, int column_index,
  std::shared_ptr<arrow::Buffer>* buffer) {
  std::shared_ptr<parquet::RowGroupReader> row_group_reader =
    file_reader_->RowGroup(row_group_index);
  
  *buffer = row_group_reader->GetColumnBuffer(column_index);
  return Status::OK();
}

Status ParquetRawDataReader::GetRowGroupReader(int i,
    std::shared_ptr<parquet::RowGroupReader>* row_group_reader) {
  *row_group_reader = file_reader_->RowGroup(i);
  return Status::OK();
}

Status GetColumnReader(int i, std::shared_ptr<parquet::ColumnReader>* column_reader) {
  return Status::OK();
}


} // namespace pegasus