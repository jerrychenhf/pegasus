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

#include "catalog/spark_catalog.h"
#include "runtime/planner_exec_env.h"

using namespace std;

namespace pegasus {

SparkCatalog::SparkCatalog() {
 PlannerExecEnv* env =  PlannerExecEnv::GetInstance();
 std::shared_ptr<StoragePluginFactory> storage_plugin_factory_ = 
     env->get_storage_plugin_factory();
}

SparkCatalog::~SparkCatalog() {
    
}

Status SparkCatalog::GetPartitions(DataSetRequest* dataset_request,
    std::shared_ptr<std::vector<Partition>>* partitions) {
  
  std::string table_location = dataset_request->get_dataset_path();
  std::shared_ptr<StoragePlugin> storage_plugin_;
  RETURN_IF_ERROR(storage_plugin_factory_->GetStoragePlugin(table_location, &storage_plugin_));
  std::vector<std::string> file_list;
  RETURN_IF_ERROR(storage_plugin_->ListFiles(table_location, &file_list));
  for (auto filepath : file_list) {
    partitions->get()->push_back(Partition(Identity(table_location, filepath)));
  }
  return Status::OK();
}
} // namespace pegasus
