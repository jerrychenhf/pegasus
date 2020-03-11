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

#include "test/gtest-util.h"
#include "storage/storage_plugin.h"
#include "storage/storage_plugin_factory.h"

namespace pegasus {

TEST(StoragePluginTest, Unit) {

  std::shared_ptr<StoragePluginFactory> planner_storage_plugin_factory(
      new StoragePluginFactory());

  std::shared_ptr<StoragePlugin> planner_storage_plugin;
  //TODO: create a test file.
  std::string table_location = "hdfs://10.239.47.55:9000/genData1000/customer";
  ASSERT_OK(planner_storage_plugin_factory->GetStoragePlugin(table_location,
      &planner_storage_plugin));

  ASSERT_NE(nullptr, planner_storage_plugin);
  ASSERT_EQ(StoragePlugin::HDFS, planner_storage_plugin->GetPluginType());

  auto partitions = std::make_shared<std::vector<Partition>>();

  std::vector<std::string> file_list;
  ASSERT_OK(planner_storage_plugin->ListFiles(table_location, &file_list));
  // ASSERT_EQ(1, file_list.size());
  for (auto filepath : file_list) {
    partitions->push_back(Partition(Identity(table_location, filepath)));
  }
  
  std::shared_ptr<StoragePluginFactory> worker_storage_plugin_factory(
      new StoragePluginFactory());

  std::shared_ptr<StoragePlugin> worker_storage_plugin;
  for(auto partition : *partitions) {
    std::string partition_path = partition.GetIdentPath();
    ASSERT_OK(worker_storage_plugin_factory->GetStoragePlugin(partition_path,
        &worker_storage_plugin));
    ASSERT_NE(nullptr, worker_storage_plugin);
    ASSERT_EQ(StoragePlugin::HDFS, worker_storage_plugin->GetPluginType());
    arrow::internal::Uri uri;
    uri.Parse(partition_path);
    std::string file_path = uri.path();

    std::shared_ptr<HdfsReadableFile> file;
    ASSERT_OK(worker_storage_plugin->GetReadableFile(file_path, &file));
  }
}

}

PEGASUS_TEST_MAIN();

