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
#include <arrow/record_batch.h>
#include <arrow/type.h>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#include "catalog/spark_catalog.h"
#include "parquet/parquet_reader.h"
#include "runtime/planner_exec_env.h"

using namespace std;

namespace pegasus {

const std::string SparkCatalog::FILE_FORMAT_ID_PARQUET = "PARQUET";

SparkCatalog::SparkCatalog() : storage_plugin_factory_(new StoragePluginFactory()) {

}

SparkCatalog::~SparkCatalog() {
    
}

Status SparkCatalog::GetTableLocation(DataSetRequest* dataset_request, std::string& table_location) {
  const auto properties = dataset_request->get_properties();
  std::unordered_map<std::string, std::string>::const_iterator it = 
      properties.find(DataSetRequest::TABLE_LOCATION);
  if (it != properties.end()) {
    table_location = it->second;
    return Status::OK();
  } else {
    return Status::ObjectNotFound("table path not found.");
  }
}

Status SparkCatalog::GetSchema(DataSetRequest* dataset_request,
    std::shared_ptr<arrow::Schema>* schema) {
  
  std::string table_location;
  RETURN_IF_ERROR(GetTableLocation(dataset_request, table_location));
  std::shared_ptr<StoragePlugin> storage_plugin;
  RETURN_IF_ERROR(storage_plugin_factory_->GetStoragePlugin(table_location, &storage_plugin));
  std::vector<std::string> file_list;
  RETURN_IF_ERROR(storage_plugin->ListFiles(table_location, &file_list));

  if (storage_plugin->GetPluginType() == StoragePlugin::HDFS) {
    std::shared_ptr<HdfsReadableFile> file;
    RETURN_IF_ERROR(std::dynamic_pointer_cast<HDFSStoragePlugin>(storage_plugin)
        ->GetReadableFile(file_list[0], &file));

    parquet::ArrowReaderProperties properties = parquet::default_arrow_reader_properties();
    arrow::MemoryPool *pool = arrow::default_memory_pool();
    std::unique_ptr<ParquetReader> parquet_reader(new ParquetReader(file, pool, properties));
    RETURN_IF_ERROR(parquet_reader->GetSchema(schema));
  }

  return Status::OK();
}

Catalog::FileFormat SparkCatalog::GetFileFormat(DataSetRequest* dataset_request) {
  const auto properties = dataset_request->get_properties();
  std::unordered_map<std::string, std::string>::const_iterator it = 
      properties.find(DataSetRequest::FORMAT);
  if (it != properties.end()) {
    if (it->second == SparkCatalog::FILE_FORMAT_ID_PARQUET) {
      return FileFormat::PARQUET;
    } else {
      return FileFormat::UNKNOWN;
    }
  } else {
    return FileFormat::PARQUET;
  }
}


Catalog::CatalogType SparkCatalog::GetCatalogType() {
   return CatalogType::SPARK;
}

} // namespace pegasus
