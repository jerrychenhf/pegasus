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
#include <gtest/gtest.h>

#include "arrow/io/hdfs.h"
#include "arrow/util/uri.h"
#include "test/gtest-util.h"
#include "storage/storage.h"
#include "storage/storage_factory.h"

namespace pegasus {

TEST(StorageTest, Unit) {

  std::shared_ptr<StorageFactory> planner_storage_factory(
      new StorageFactory());

  std::shared_ptr<Storage> planner_storage;
  //TODO: create a test file.
  std::string table_location = "hdfs://10.239.47.55:9000/genData1000/customer";
  ASSERT_OK(planner_storage_factory->GetStorage(table_location,
      &planner_storage));

  ASSERT_NE(nullptr, planner_storage);
  ASSERT_EQ(Storage::HDFS, planner_storage->GetStorageType());

  uint64_t time;
  ASSERT_OK(planner_storage->GetModifedTime(table_location, &time));
  ASSERT_NE(0, time);

  auto partitions = std::make_shared<std::vector<Partition>>();

  std::vector<std::string> file_list;
  int64_t total_bytes;
  ASSERT_OK(planner_storage->ListFiles(table_location, &file_list, &total_bytes));
  ASSERT_NE(0, total_bytes);
  // ASSERT_EQ(1, file_list.size());
  for (auto filepath : file_list) {
    partitions->push_back(Partition(Identity(table_location, filepath)));
  }
  
  std::shared_ptr<StorageFactory> worker_storage_factory(
      new StorageFactory());

  std::shared_ptr<Storage> worker_storage;
  for(auto partition : *partitions) {
    std::string partition_path = partition.GetIdentityPath();
    ASSERT_OK(worker_storage_factory->GetStorage(partition_path,
        &worker_storage));
    ASSERT_NE(nullptr, worker_storage);
    ASSERT_EQ(Storage::HDFS, worker_storage->GetStorageType());
    arrow::internal::Uri uri;
    uri.Parse(partition_path);
    std::string file_path = uri.path();

    std::shared_ptr<HdfsReadableFile> file;
    ASSERT_OK(std::dynamic_pointer_cast<HDFSStorage>(worker_storage)
        ->GetReadableFile(file_path, &file));
  }
}

}

PEGASUS_TEST_MAIN();

