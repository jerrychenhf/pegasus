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

//TODO use the concurrent_hash_map
//#include "tbb/concurrent_hash_map.h"
//using namespace tbb;

#include <memory>
#include <unordered_map>

#include "catalog/pegasus_catalog.h"
#include "catalog/spark_catalog.h"
#include "dataset/dataset_service.h"
#include "dataset/flightinfo_builder.h"
#include "consistent_hashing.h"

DECLARE_bool(check_dataset_append_enabled);

namespace pegasus
{

DataSetService::DataSetService()
{
}

DataSetService::~DataSetService()
{
}

Status DataSetService::Init()
{
  //  PlannerExecEnv* env =  PlannerExecEnv::GetInstance();
  dataset_store_ = std::unique_ptr<DataSetStore>(new DataSetStore);
  //  worker_manager_ = env->GetInstance()->get_worker_manager();
  catalog_manager_ = std::make_shared<CatalogManager>();
  PlannerExecEnv::GetInstance()->get_worker_manager()->RegisterObserver(this);

  return Status::OK();
}

void DataSetService::WkMngObsUpdate(int wmevent)
{
  if ((WMEVENT_WORKERNODE_ADDED == wmevent) || (WMEVENT_WORKERNODE_REMOVED == wmevent))
    dataset_store_->InvalidateAll();
}

Status DataSetService::GetDataSets(std::shared_ptr<std::vector<std::shared_ptr<DataSet>>> *datasets)
{

  dataset_store_->GetDataSets(datasets);
  return Status::OK();
}

Status DataSetService::NotifyDataCacheDrop(std::shared_ptr<DataSet> pds, std::shared_ptr<std::vector<Partition>> partitions)
{
  // generate the list of partitions which needs to notify workernode to drop the cached data
  LOG(INFO) << "Generating list of partitions to drop cached data...";
  auto partstodrop = std::make_shared<std::vector<Partition>>();
  for (auto pttold : pds->partitions())
  {
    for (auto ptit = partitions->begin(); ptit != partitions->end(); ptit++)
    {
      if ((pttold.GetIdentPath() == ptit->GetIdentPath()) && (pttold.GetLocationURI() != ptit->GetLocationURI()))
      {
        partstodrop->push_back(pttold);
        break;
      }
    }
  }
  LOG(INFO) << "Generated drop list (locationuri partitionid):";
  for (auto ptt : *partstodrop)
  {
    LOG(INFO) << ptt.GetLocationURI() << "\t" << ptt.GetIdentPath();
  }
  PlannerExecEnv::GetInstance()->get_worker_manager()->UpdateCacheDropLists(partstodrop);

  return Status::OK();
}

Status DataSetService::RefreshDataSet(DataSetRequest *dataset_request, std::string table_location, std::shared_ptr<StoragePlugin> storage_plugin, std::shared_ptr<DataSet> pds, std::shared_ptr<DataSet> *dataset)
{
  auto partitions = std::make_shared<std::vector<Partition>>();

  LOG(INFO) << "pds->getRefreshFlag(): " << pds->getRefreshFlag();
  if (pds->getRefreshFlag() & DSRF_FILES_APPEND)
  {
    LOG(INFO) << "=== DSRF_FILES_APPEND";
    auto dsbuilder = std::make_shared<DataSetBuilder>(catalog_manager_);
    dsbuilder->BuildDatasetPartitions(table_location, storage_plugin, partitions, CONHASH);
  }
  else if (pds->getRefreshFlag() & DSRF_WORKERSET_CHG)
  {
    LOG(INFO) << "=== DSRF_WORKERSET_CHG";
    auto distributor = std::make_shared<ConsistentHashRing>();
    distributor->PrepareValidLocations(nullptr, nullptr);
    distributor->SetupDist();
    for (auto ptt : pds->partitions())
    {
      Partition partition = Partition(Identity(pds->dataset_path(), ptt.GetIdentPath()));
      partitions->push_back(partition);
    }
    distributor->GetDistLocations(partitions);

    NotifyDataCacheDrop(pds, partitions);
  }
  else
  {
    LOG(ERROR) << "Unknown dataset refresh flag type!";
  }

  // keep the existing dataset pointer in dataset_store, only update data
  pds->lockwrite();
  //replacePartitions(std::vector<Partition> partits)
  pds->replacePartitions(*partitions);
  pds->resetRefreshFlag();
  *dataset = pds;
  pds->unlockwrite();

  return Status::OK();
}

Status DataSetService::GetDataSet(DataSetRequest *dataset_request, std::shared_ptr<DataSet> *dataset)
{

  std::shared_ptr<DataSet> pds = NULL;
  std::string dataset_path = dataset_request->get_dataset_path();
  dataset_store_->GetDataSet(dataset_path, &pds);

  if (pds == NULL)
  {
    LOG(INFO) << "=== Not found, creating new dataset ...";
    // === CacheDataSet(dataset_path, dataset, CONHASH);
    // build the dataset and insert it to dataset store.
    auto dsbuilder = std::make_shared<DataSetBuilder>(catalog_manager_);
    // Status BuildDataset(std::shared_ptr<DataSet>* dataset);
    dsbuilder->BuildDataset(dataset_request, dataset, CONHASH);
    // Begin Write
    (*dataset)->lockwrite();
#if 0
    // read again to avoid duplicated write
    dataset_store_->GetDataSet(dataset_path, &pds);
    if (pds != NULL)
    {
      (*dataset)->unlockwrite();  // drop this prepared dataset
      *dataset = std::make_shared<DataSet>(pds->GetData()); // fill dataset from pds
      pds->unlockread();
      return Status::OK();
    }
#endif
    // do the write
    dataset_store_->InsertDataSet(std::shared_ptr<DataSet>(*dataset));
    (*dataset)->unlockwrite();
    // End Write
  }
  else
  {
    LOG(INFO) << "=== Found, check timestamp and update refresh flag...";
    // check timestamp and update refresh flag
    uint64_t timestamp = 0;

    std::shared_ptr<Catalog> catalog;
    std::string table_location;
    std::shared_ptr<StoragePlugin> storage_plugin;
    RETURN_IF_ERROR(catalog_manager_->GetCatalog(dataset_request, &catalog));
    LOG(INFO) << "Getting storage plugin ...";
    if (catalog->GetCatalogType() == Catalog::SPARK) {
      RETURN_IF_ERROR(catalog->GetTableLocation(dataset_request, table_location));
      RETURN_IF_ERROR(PlannerExecEnv::GetInstance()->get_storage_plugin_factory()->GetStoragePlugin(table_location, &storage_plugin));
    }
    LOG(INFO) << "Got.";

    if (FLAGS_check_dataset_append_enabled) {
      storage_plugin->GetModifedTime(pds->dataset_path(), &timestamp);
      LOG(INFO) << "timestamp: " << timestamp;
      if (timestamp > pds->getTimestamp()) {
        LOG(INFO) << "=== filesystem timestamp changed, set refresh flag";
        pds->lockwrite();
        pds->setRefreshFlag(DSRF_FILES_APPEND);
        LOG(INFO) << "pds->getRefreshFlag(): " << pds->getRefreshFlag();
        pds->setTimestamp(timestamp);
        pds->unlockwrite();
      }
    }

    // if need refresh
    if (pds->needRefresh())
    {
      LOG(INFO) << "=== Need to refresh, refreshing ...";
      RefreshDataSet(dataset_request, table_location, storage_plugin, pds, dataset);
    }    //if need refresh
    else // found and is uptodate
    {
      LOG(INFO) << "=== Up-to-date";
      pds->lockread();
      //    *dataset = std::shared_ptr<DataSet>(new DataSet(*pds));
      *dataset = std::make_shared<DataSet>(pds->GetData());
      (*dataset)->set_schema(pds->get_schema());
      pds->unlockread();
    }
  } // pds is not NULL

  LOG(INFO) << "DataSetService::GetDataSet() finished successfully.";

  return Status::OK();
}

Status DataSetService::CacheDataSet(DataSetRequest *dataset_request, std::shared_ptr<DataSet> *dataset, int distpolicy)
{

  // build the dataset and insert it to dataset store.
  auto dsbuilder = std::make_shared<DataSetBuilder>(catalog_manager_);
  // Status BuildDataset(std::shared_ptr<DataSet>* dataset);
  RETURN_IF_ERROR(dsbuilder->BuildDataset(dataset_request, dataset, distpolicy));
  // Begin Write
  (*dataset)->lockwrite();
  dataset_store_->InsertDataSet(std::shared_ptr<DataSet>(*dataset));
  (*dataset)->unlockwrite();
  // End Write

  return Status::OK();
}

Status DataSetService::RemoveDataSet(DataSetRequest *dataset_request)
{

  std::shared_ptr<DataSet> pds = NULL;
  std::string dataset_path = dataset_request->get_dataset_path();
  dataset_store_->GetDataSet(dataset_path, &pds);
  //Status DataSetStore::RemoveDataSet(std::shared_ptr<DataSet> dataset)
  pds->lockwrite();
  dataset_store_->RemoveDataSet(pds);
  pds->unlockwrite();

  return Status::OK();
}

/// Build FlightInfo from DataSet.
Status DataSetService::GetFlightInfo(DataSetRequest *dataset_request,
                                     std::unique_ptr<rpc::FlightInfo> *flight_info,
                                     const rpc::FlightDescriptor &fldtr)
{

  LOG(INFO) << "GetFlightInfo()...";
  std::shared_ptr<DataSet> dataset = nullptr;
  RETURN_IF_ERROR(GetDataSet(dataset_request, &dataset));

  LOG(INFO) << "Filtering the dataSet";
  std::shared_ptr<ResultDataSet> rdataset;
  // Filter dataset
  dataset->lockread();
  Status st = FilterDataSet(dataset_request->get_filters(), dataset, &rdataset);
  dataset->unlockread();
  // Note: we can also release the dataset readlock here, the benefit is it avoids dataset mem copy.
  if (!st.ok())
  {
    return st;
  }

  // map the column names to column indices
  LOG(INFO) << "Mapping the column names to column indices";
  std::vector<std::string> column_names = dataset_request->get_column_names();
  std::vector<int32_t> column_indices;
  std::shared_ptr<arrow::Schema> schema = rdataset->get_schema();

  if (column_names.empty())
  {
    column_names = schema->field_names();
  }

  arrow::SchemaBuilder builder;
  for (std::string column_name : column_names)
  {
    int32_t index = schema->GetFieldIndex(column_name);
    if (index != -1)
    {
      column_indices.push_back(index);
      std::shared_ptr<arrow::Field> field = schema->GetFieldByName(column_name);
      if (nullptr != field)
      {
        builder.AddField(field);
      }
      else
      {
        Status::Invalid("column name: ", column_name, "can't find in table.");
      }
    }
    else
    {
      Status::Invalid("column name: ", column_name, "can't find in table.");
    }
  }

  std::shared_ptr<arrow::Schema> new_schema;
  arrow::Result<std::shared_ptr<arrow::Schema>> result = builder.Finish();
  if (result.ok())
  {
    new_schema = result.ValueOrDie();
  }
  else
  {
    Status::Invalid("Failed to get new schema.");
  }

  // std::shared_ptr<arrow::Schema> new_schema = std::make_shared<arrow::Schema>(fields);

  dataset_request->set_column_indices(column_indices);

  LOG(INFO) << "Building flightinfo";
  flightinfo_builder_ = std::shared_ptr<FlightInfoBuilder>(new FlightInfoBuilder(rdataset));
  RETURN_IF_ERROR(flightinfo_builder_->BuildFlightInfo(flight_info, new_schema, column_indices, (rpc::FlightDescriptor &)fldtr));
  return Status::OK();
}

Status DataSetService::FilterDataSet(const std::vector<Filter> &parttftr, std::shared_ptr<DataSet> dataset,
                                     std::shared_ptr<ResultDataSet> *resultdataset)
{
  LOG(INFO) << "FilterDataSet()...";
  //TODO: filter the dataset
  ResultDataSet::Data rdata;
  rdata.dataset_path = dataset->dataset_path();
  rdata.partitions = dataset->partitions();
  rdata.total_bytes = dataset->total_bytes();
  rdata.total_records = dataset->total_records();

  *resultdataset = std::make_shared<ResultDataSet>(std::move(rdata));
  (*resultdataset)->set_schema(dataset->get_schema());

  LOG(INFO) << "...FilterDataSet()";
  return Status::OK();
}

/// Build FlightInfos from DataSets.
Status DataSetService::GetFlightListing(std::unique_ptr<rpc::FlightListing> *listings)
{

  std::shared_ptr<std::vector<std::shared_ptr<DataSet>>> datasets;
  GetDataSets(&datasets);

  auto rdatasets = std::make_shared<std::vector<std::shared_ptr<ResultDataSet>>>();
  //TODO: fill the resultdataset
  /*  for (auto ds:(*datasets))
  {
    ResultDataSet::Data dd;
    dd.dataset_path = ds->dataset_path();
    dd.partitions = ds->partitions();
    dd.total_bytes = ds->total_bytes();
    dd.total_records = ds->total_records();
    rdatasets->push_back(&ResultDataSet(dd));
  }
*/
  flightinfo_builder_ = std::shared_ptr<FlightInfoBuilder>(new FlightInfoBuilder(rdatasets));
  flightinfo_builder_->BuildFlightListing(listings);
  return Status::OK();
}

} // namespace pegasus