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

#include "pegasus/catalog/spark_catalog.h"
#include "pegasus/dataset/dataset_builder.h"
#include "pegasus/parquet/parquet_metadata.h"

namespace pegasus {

DataSetBuilder::DataSetBuilder(std::string dataset_path, std::shared_ptr<std::vector<std::string>> file_list)
  : file_list_(file_list) {

}

Status DataSetBuilder::BuildDataset(std::shared_ptr<DataSet>* dataset) {
  std::unique_ptr<SparkCatalog> spark_catalog = std::unique_ptr<SparkCatalog>(new SparkCatalog(dataset_path));
  if (spark_catalog->GetFileFormat() == "parquet") {
    std::shared_ptr<ParquetMetadata> table_meta = std::shared_ptr<ParquetMetadata>(new ParquetMetadata(file_list_));
    table_meta->GetFilesMeta(files_metadata_);
   }
}

Status DataSetBuilder::GetTotalRecords(int64_t* total_records) {

 for(auto it = (*files_metadata_)->begin(); it != (*files_metadata_)->end(); it++) {
  int64_t num_rows = it->num_rows();
  *total_records += num_rows;
 }

}

} // namespace pegasus