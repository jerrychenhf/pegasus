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

#ifndef PEGASUS_PARQUET_READER_H
#define PEGASUS_PARQUET_READER_H

#include "arrow/io/interfaces.h"
#include "parquet/arrow/reader.h"

#include "common/status.h"

namespace pegasus {

class ParquetReader {
 public:
  ParquetReader(const std::shared_ptr<arrow::io::RandomAccessFile>& file,
                arrow::MemoryPool* pool, const parquet::ArrowReaderProperties& properties);
  Status ReadParquetTable(std::shared_ptr<arrow::Table>* table);
  Status ReadParquetTable(const std::vector<int> column_indices, std::shared_ptr<::arrow::Table>* table);
  Status ReadColumnChunk(int column_index, std::shared_ptr<arrow::ChunkedArray>* chunked_out);
  Status ReadColumnChunk(int column_index,std::shared_ptr<arrow::ChunkedArray>* chunked_out, int size);
  Status ReadColumnChunk(int64_t row_group_index, int column_id, std::shared_ptr<arrow::ChunkedArray>* chunked_out);

 private:
  std::unique_ptr< parquet::arrow::FileReader> file_reader_;
};

} // namespace pegasus

#endif  // PEGASUS_PARQUET_READER_H