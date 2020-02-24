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

#include "parquet/parquet_reader.h"

namespace pegasus {

ParquetReader::ParquetReader(const std::shared_ptr<arrow::io::RandomAccessFile>& file,
                             arrow::MemoryPool* pool,
                             const parquet::ArrowReaderProperties& properties) {

  parquet::arrow::FileReaderBuilder builder;
  builder.Open(std::move(file));
  builder.memory_pool(pool)->properties(properties)->Build(&file_reader_);
}

Status ParquetReader::GetSchema(std::shared_ptr<arrow::Schema>* out) {
  arrow::Status arrowStatus = file_reader_->GetSchema(out);
  Status status = Status::fromArrowStatus(arrowStatus);
  RETURN_IF_ERROR(status);
  return Status::OK();
}

Status ParquetReader::ReadParquetTable(std::shared_ptr<arrow::Table>* table) {
  arrow::Status arrowStatus = file_reader_->ReadTable(table); 
  Status status = Status::fromArrowStatus(arrowStatus);
  RETURN_IF_ERROR(status);
  return Status::OK();
}

Status ParquetReader::ReadParquetTable(const std::vector<int> column_indices, std::shared_ptr<::arrow::Table>* table) {
  arrow::Status arrowStatus = file_reader_->ReadTable(column_indices, table); 
  Status status = Status::fromArrowStatus(arrowStatus);
  RETURN_IF_ERROR(status);
  return Status::OK();
}

Status ParquetReader::ReadColumnChunk(int column_indice, std::shared_ptr<arrow::ChunkedArray>* chunked_out) {

  arrow::Status arrowStatus = file_reader_->ReadColumn(column_indice, chunked_out);
  Status status = Status::fromArrowStatus(arrowStatus);
  RETURN_IF_ERROR(status);
  return Status::OK();
}

Status ParquetReader::ReadColumnChunk(int column_indice, std::shared_ptr<arrow::ChunkedArray>* chunked_out, int size) {

  std::unique_ptr<parquet::arrow::ColumnReader> column_reader;

  arrow::Status arrowStatus;
  Status status;
  arrowStatus = file_reader_->GetColumn(column_indice, &column_reader);
  status = Status::fromArrowStatus(arrowStatus);
  RETURN_IF_ERROR(status);

  arrowStatus = column_reader->NextBatch(size, chunked_out);
  status = Status::fromArrowStatus(arrowStatus);
  RETURN_IF_ERROR(status);

  return Status::OK();
}

Status ParquetReader::ReadColumnChunk(int64_t row_group_index, int column_id, std::shared_ptr<arrow::ChunkedArray>* chunked_out) {

  std::shared_ptr<parquet::arrow::RowGroupReader> row_group_reader = file_reader_->RowGroup(row_group_index);
  std::shared_ptr<parquet::arrow::ColumnChunkReader> column_chunk_reader = row_group_reader->Column(column_id);
  
  arrow::Status arrowStatus = column_chunk_reader->Read(chunked_out);
  Status status = Status::fromArrowStatus(arrowStatus);
  RETURN_IF_ERROR(status);

  return Status::OK();
}


} // namespace pegasus