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

#include <stdlib.h>
#include <stdio.h>
#include <iostream>

#include "arrow/util/uri.h"
#include <gtest/gtest.h>

#include "parquet/parquet_reader.h"
#include "test/gtest-util.h"
#include "storage/storage_plugin.h"
#include "storage/storage_plugin_factory.h"

namespace pegasus {

TEST(ParquetReaderTest, Unit) {

  std::string partition_path = 
      "hdfs://10.239.47.55:9000/genData2/customer/part-00000-1fafbf9f-6edf-4f8f-8b51-268708b6f6c5-c000.snappy.parquet";
  std::shared_ptr<StoragePluginFactory> worker_storage_plugin_factory(
      new StoragePluginFactory());

  std::shared_ptr<StoragePlugin> worker_storage_plugin;

  ASSERT_OK(worker_storage_plugin_factory->GetStoragePlugin(partition_path, &worker_storage_plugin));
  ASSERT_NE(nullptr, worker_storage_plugin);
  ASSERT_EQ(StoragePlugin::HDFS, worker_storage_plugin->GetPluginType());

  std::shared_ptr<HdfsReadableFile> file;
  ASSERT_OK(worker_storage_plugin->GetReadableFile(partition_path, &file));

  parquet::ArrowReaderProperties properties = parquet::default_arrow_reader_properties();
  // static parquet::ArrowReaderProperties properties;

  arrow::MemoryPool *pool = arrow::default_memory_pool();
  std::unique_ptr<ParquetReader> parquet_reader(new ParquetReader(file, pool, properties));

  std::shared_ptr<arrow::Table> table1;
  ASSERT_OK(parquet_reader->ReadParquetTable(&table1));
  ASSERT_NE(nullptr, table1);
  ASSERT_EQ(18, table1->num_columns());
  ASSERT_EQ(144000, table1->num_rows());

  std::shared_ptr<arrow::Table> table2;
  std::vector<int> column_indices;
  column_indices.push_back(0);
  ASSERT_OK(parquet_reader->ReadParquetTable(column_indices, &table2));
  ASSERT_NE(nullptr, table2);
  ASSERT_EQ(1, table2->num_columns());
  ASSERT_EQ(144000, table2->num_rows());

  std::shared_ptr<arrow::ChunkedArray> chunked_out1;
  ASSERT_OK(parquet_reader->ReadColumnChunk(0, 0, &chunked_out1));
  ASSERT_EQ(144000, chunked_out1->length());
  ASSERT_EQ(1, chunked_out1->num_chunks());

  std::shared_ptr<arrow::ChunkedArray> chunked_out2;
  ASSERT_OK(parquet_reader->ReadColumnChunk(0, &chunked_out2, 2));
  ASSERT_EQ(2, chunked_out2->length());
  ASSERT_EQ(1, chunked_out2->num_chunks());

  std::shared_ptr<arrow::ChunkedArray> chunked_out3;
  ASSERT_OK(parquet_reader->ReadColumnChunk(0, &chunked_out3));
  ASSERT_EQ(144000, chunked_out3->length());
  ASSERT_EQ(1, chunked_out3->num_chunks());

}
}

PEGASUS_TEST_MAIN();