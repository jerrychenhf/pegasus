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

#include "pegasus/dataset/dataset_builder.h"
#include "pegasus/parquet/parquet_metadata.h"

namespace pegasus {

DataSetBuilder::DataSetBuilder(std::string dataset_path, std::shared_ptr<std::vector<std::string>> file_list, std::shared_ptr<std::vector<Location>> vectloc)
  : file_list_(file_list), vectloc_(vectloc) {

}

Status DataSetBuilder::BuildDataset(std::shared_ptr<DataSet>* dataset) {
  DataSet::Data dd;
  dd.dataset_path = dataset_path;
  for (size_t i=0; i<file_list_->size(); i++)
  {
    // create Identity
    Identity id((*file_list_)[i], 0, 0, 0);
    // create Location
    Location loc(vectloc_->at(i));
    // create Partition
    Partition ep(id, loc);
    dd.partitions.push_back(ep);
  }
  *dataset = std::make_shared<DataSet>(dd);

  return Status::OK();
}

Status DataSetBuilder::GetTotalRecords(int64_t* total_records) {
  *total_records = file_list_->size();  //TODO: need to confirm
  return Status::OK();
}

} // namespace pegasus