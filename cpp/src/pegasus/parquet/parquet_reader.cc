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

#include "pegasus/parquet/parquet_reader.h"

#include "parquet/properties.h"
#include "arrow/table.h"

namespace pegasus {

ParquetReader::ParquetReader(const std::shared_ptr<arrow::io::RandomAccessFile>& file) {

  static parquet::ArrowReaderProperties default_reader_props;
  parquet::arrow::FileReaderBuilder builder;
  builder.Open(file);
  builder.properties(default_reader_props)->Build(&file_reader_);
}

Status ParquetReader::ReadTable(std::shared_ptr<arrow::Table> table) {

  file_reader_->ReadTable(&table); 
}

Status ParquetReader::ReadColumnChunk(int column_index, std::shared_ptr<arrow::ChunkedArray> out) {

  file_reader_->ReadColumn(column_index, &out);
}

Status ParquetReader::ReadColumnChunk(int column_index, int size, std::shared_ptr<arrow::ChunkedArray> chunked_out) {

  std::unique_ptr<parquet::arrow::ColumnReader> column_reader;
  file_reader_->GetColumn(column_index, &column_reader);
  column_reader->NextBatch(size, &chunked_out);
}

Status ParquetReader::ReadColumnChunk(int64_t row_group_index, int column_id, std::shared_ptr<arrow::ChunkedArray> chunked_out) {

  std::shared_ptr<parquet::arrow::RowGroupReader> row_group_reader = file_reader_->RowGroup(row_group_index);
  std::shared_ptr<parquet::arrow::ColumnChunkReader> column_chunk_reader = row_group_reader->Column(column_id);
  column_chunk_reader->Read(&chunked_out);
}


} // namespace pegasus