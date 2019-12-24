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

#ifndef PEGASUS_DATASET_SERVICE_H
#define PEGASUS_DATASET_SERVICE_H

#include <string>

#include "pegasus/dataset/dataset.h"
#include "pegasus/dataset/dataset_store.h"
#include "pegasus/dataset/flightinfo_builder.h"
#include "pegasus/runtime/exec_env.h"
#include "pegasus/storage/storage_plugin_factory.h"
#include "pegasus/common/worker_manager.h"

using namespace std;

namespace pegasus {

class DataSetService {
 public:
  DataSetService();
  ~DataSetService();
  Status Init();
  Status Start();
  Status Stop();
  Status GetFlightInfo(std::string dataset_path, std::unique_ptr<FlightInfo>* flight_info);
  Status GetFlightListing(std::unique_ptr<FlightListing>* listings);
  Status GetDataSets( std::shared_ptr<std::vector<std::shared_ptr<DataSet>>>* datasets);
  Status GetDataSet(std::string dataset_path, std::shared_ptr<DataSet>* dataset);

 private:
  std::shared_ptr<StoragePlugin>* storage_plugin_;
  std::shared_ptr<FlightInfoBuilder> flightinfo_builder_;
  std::shared_ptr<WorkerManager> worker_manager_;
  std::shared_ptr<DataSetStore> dataset_store_;
};

} // namespace pegasus

#endif  // PEGASUS_DATASET_SERVICE_H