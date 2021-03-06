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

#include <arrow/result.h>

#include "catalog/catalog_manager.h"
#include "dataset/dataset.h"
#include "dataset/dataset_store.h"
#include "dataset/dataset_builder.h"
#include "dataset/dataset_request.h"
#include "dataset/flightinfo_builder.h"
#include "dataset/filter.h"
#include "runtime/planner_exec_env.h"
#include "server/planner/worker_manager.h"

using namespace std;

namespace pegasus
{

namespace rpc
{

class FlightInfo;
class FlightListing;

} // namespace rpc

class DataSetService : WorkerManagerObserver
{
public:
  DataSetService();
  ~DataSetService();

  Status Init();
  Status Start();
  Status Stop();

  Status GetFlightInfo(DataSetRequest *dataset_request,
    std::unique_ptr<rpc::FlightInfo> *flight_info,
    const rpc::FlightDescriptor &fldtr);
  Status GetFlightListing(std::unique_ptr<rpc::FlightListing> *listings);
  Status GetDataSets(std::shared_ptr<std::vector<std::shared_ptr<DataSet>>> *datasets);
  Status GetDataSet(DataSetRequest *dataset_request, std::shared_ptr<DataSet> *dataset);
private:
  Status NotifyDataCacheDrop(std::shared_ptr<DataSet> pds,
    std::shared_ptr<std::vector<Partition>> partitions);
  Status RefreshDataSet(DataSetRequest *dataset_request,
    std::string table_location, std::shared_ptr<Storage> storage,
      std::shared_ptr<DataSet> pds, std::shared_ptr<DataSet> *dataset);
  Status CacheDataSet(DataSetRequest *dataset_request,
    std::shared_ptr<DataSet> *dataset, int distpolicy);
  Status RemoveDataSet(DataSetRequest *dataset_request);
  Status FilterDataSet(const std::vector<Filter> &parttftr,
    std::shared_ptr<DataSet> dataset, std::shared_ptr<ResultDataSet> *resultdataset);
    
  void ObserverUpdate(int wmevent);

  Status GetColumnIndices(DataSetRequest *dataset_request,
                          std::shared_ptr<arrow::Schema> schema,
                          std::shared_ptr<std::vector<int32_t>>* column_indices,
                          std::shared_ptr<arrow::Schema>* new_schema);

private:
  std::shared_ptr<DataSetStore> dataset_store_;
  std::shared_ptr<CatalogManager> catalog_manager_;
};

} // namespace pegasus

#endif // PEGASUS_DATASET_SERVICE_H