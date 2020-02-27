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
#include "parquet/parquet_metadata.h"
#include "runtime/planner_exec_env.h"

namespace pegasus {

DataSetBuilder::DataSetBuilder(std::shared_ptr<CatalogManager> catalog_manager)
    : catalog_manager_(catalog_manager) {
  PlannerExecEnv* env =  PlannerExecEnv::GetInstance();
  storage_plugin_factory_ = env->get_storage_plugin_factory();
}

Status DataSetBuilder::BuildDataset(DataSetRequest* dataset_request,
                                   std::shared_ptr<DataSet>* dataset, int distpolicy) {

#if 0 //TODO: need redesign
  std::shared_ptr<DSDistributor> distributor;
  switch (distpolicy)
  {
    case CONHASH:
//      distributor = std::make_shared<DSDistributor>(new ConsistentHashRing());
      distributor = std::static_pointer_cast<DSDistributor>(std::make_shared<ConsistentHashRing>());
      break;
    case LOCALONLY:
      distributor = std::static_pointer_cast<DSDistributor>(std::make_shared<DistLocalOnly>());
      break;
    case LOCALPREFER:
      distributor = std::static_pointer_cast<DSDistributor>(std::make_shared<DistLocalPrefer>());
      break;
    default:
      return Status::NotImplemented("Distributor Type");
  }
#endif
  //TODO: only consider ConsistentHashRing for now
//  std::shared_ptr<DSDistributor> distributor = std::make_shared<ConsistentHashRing>();  //error: conversion from ‘...’ to non-scalar type 
//  std::shared_ptr<DSDistributor> distributor;
//  distributor = std::static_pointer_cast<DSDistributor>(std::make_shared<ConsistentHashRing>()); //error: is an inaccessible base of
//  DSDistributor* distributor(new ConsistentHashRing); //error: is an inaccessible base of
  auto distributor = std::make_shared<ConsistentHashRing>(); 
  // TODO: get locations here to decouple distributor from workermanager
  distributor->PrepareValidLocations(nullptr, nullptr);
  distributor->SetupDist();

  // create partitions with identities
  auto vectident = std::make_shared<std::vector<Identity>>();
  auto partitions = std::make_shared<std::vector<Partition>>();
  
  // setup the identity vector for ondisk dataset
  std::shared_ptr<Catalog> catalog;
  RETURN_IF_ERROR(catalog_manager_->GetCatalog(dataset_request, &catalog));

  std::shared_ptr<arrow::Schema> schema;

  if (catalog->GetCatalogType() == Catalog::SPARK) {
    std::string table_location;
    RETURN_IF_ERROR(catalog->GetTableLocation(dataset_request, table_location));
    std::shared_ptr<StoragePlugin> storage_plugin;
    RETURN_IF_ERROR(storage_plugin_factory_->GetStoragePlugin(table_location, &storage_plugin));
    std::vector<std::string> file_list;
    RETURN_IF_ERROR(storage_plugin->ListFiles(table_location, &file_list));

    for (auto filepath : file_list) {
      Partition partition = Partition(Identity(table_location, filepath));
      partitions->push_back(partition);
    }

    RETURN_IF_ERROR(catalog->GetSchema(dataset_request, &schema));
  } else {
    return Status::Invalid("Invalid catalog type: ", catalog->GetCatalogType());
  }

  // allocate location for each partition
//  auto vectloc = std::make_shared<std::vector<Location>>();
  distributor->GetDistLocations(partitions);
 
  // build dataset
  DataSet::Data dd;
  dd.dataset_path = dataset_request->get_dataset_path();
  for (auto partt : *partitions)
    dd.partitions.push_back(partt);

  *dataset = std::make_shared<DataSet>(dd);
  (*dataset)->set_schema(schema);

LOG(INFO) << "BuildDataset() finished successfully.";

  return Status::OK();
}

Status DataSetBuilder::GetTotalRecords(int64_t* total_records) {
//  *total_records = file_list_->size();  //TODO: need to confirm
  return Status::OK();
}

} // namespace pegasus