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

#include "dataset/dataset_builder.h"
#include "dataset/consistent_hashing.h"
#include "dataset/dataset_request.h"
#include "dataset/partition.h"
#include "runtime/planner_exec_env.h"

namespace pegasus
{

DataSetBuilder::DataSetBuilder(std::shared_ptr<CatalogManager> catalog_manager)
    : catalog_manager_(catalog_manager)
{
  PlannerExecEnv *env = PlannerExecEnv::GetInstance();
  storage_factory_ = env->get_storage_factory();
}

Status DataSetBuilder::BuildDataset(DataSetRequest *dataset_request,
                                    std::shared_ptr<DataSet> *dataset, int distpolicy)
{

  auto distributor = std::make_shared<ConsistentHashRing>();
  // TODO: get locations here to decouple distributor from workermanager
  distributor->PrepareValidLocations(nullptr, nullptr);
  distributor->SetupDist();

  // create partitions with identities
  auto partitions = std::make_shared<std::vector<Partition>>();
  int64_t total_bytes = 0;

  // setup the identity vector for ondisk dataset
  std::shared_ptr<Catalog> catalog;
  RETURN_IF_ERROR(catalog_manager_->GetCatalog(dataset_request, &catalog));

  std::shared_ptr<arrow::Schema> schema;
  uint64_t timestamp = 0;

  LOG(INFO) << "Getting catalog ...";
  if (catalog->GetCatalogType() == Catalog::SPARK)
  {
    std::string table_location;
    RETURN_IF_ERROR(catalog->GetTableLocation(dataset_request, table_location));
    std::shared_ptr<Storage> storage;
    LOG(INFO) << "Getting storage ...";
    RETURN_IF_ERROR(storage_factory_->GetStorage(table_location, &storage));
    std::vector<std::string> file_list;
    RETURN_IF_ERROR(storage->ListFiles(table_location, &file_list, &total_bytes));

    LOG(INFO) << "Filling partitions ...";
    for (auto filepath : file_list)
    {
      LOG(INFO) << "\t" << filepath;
      Partition partition = Partition(Identity(table_location, filepath));
      partitions->push_back(partition);
    }

    RETURN_IF_ERROR(catalog->GetSchema(dataset_request, &schema));

    RETURN_IF_ERROR(storage->GetModifedTime(table_location, &timestamp));
  }
  else
  {
    return Status::Invalid("Invalid catalog type: ", catalog->GetCatalogType());
  }

  LOG(INFO) << "Updating distLocations from distributor...";
  // allocate location for each partition
  distributor->GetDistLocations(partitions);

  // build dataset
  DataSet::Data dd;
  dd.total_bytes = total_bytes;
  dd.dataset_path = dataset_request->get_dataset_path();
  for (auto partt : *partitions)
    dd.partitions.push_back(partt);

  *dataset = std::make_shared<DataSet>(dd);
  (*dataset)->set_schema(schema);
  (*dataset)->setTimestamp(timestamp);

  LOG(INFO) << "BuildDataset() finished successfully.";

  return Status::OK();
}

Status DataSetBuilder::BuildDatasetPartitions(std::string table_location, std::shared_ptr<Storage> storage,
                                              std::shared_ptr<std::vector<Partition>> partitions,
                                              int distpolicy)
{
  auto distributor = std::make_shared<ConsistentHashRing>();
  // TODO: get locations here to decouple distributor from workermanager
  distributor->PrepareValidLocations(nullptr, nullptr);
  distributor->SetupDist();

  std::vector<std::string> file_list;
  RETURN_IF_ERROR(storage->ListFiles(table_location, &file_list, nullptr));

  LOG(INFO) << "Filling partitions ...";
  for (auto filepath : file_list)
  {
    LOG(INFO) << "\t" << filepath;
    Partition partition = Partition(Identity(table_location, filepath));
    partitions->push_back(partition);
  }

  LOG(INFO) << "Updating distLocations from distributor...";

  distributor->GetDistLocations(partitions);

  LOG(INFO) << "BuildDataset() finished successfully.";
  return Status::OK();
}

Status DataSetBuilder::GetTotalRecords(int64_t *total_records)
{
  //  *total_records = file_list_->size();  //TODO: need to confirm
  return Status::OK();
}

} // namespace pegasus
