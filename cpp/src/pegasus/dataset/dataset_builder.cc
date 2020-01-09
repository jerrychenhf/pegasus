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
#include "pegasus/util/consistent_hashing.h"
#include "pegasus/dataset/partition.h"

namespace pegasus {

//DataSetBuilder::DataSetBuilder() {}

Status DataSetBuilder::BuildDataset(std::string dataset_path, std::shared_ptr<DataSet>* dataset, int distpolicy) {

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
  distributor->PrepareValidLocations(nullptr);
  distributor->SetupDist();

  // create partitions with identities
    auto vectident = std::make_shared<std::vector<Identity>>();
    auto partitions = std::make_shared<std::vector<Partition>>();
    // setup the identity vector for ondisk dataset
    std::shared_ptr<std::vector<std::string>> file_list;
    //TODO: get storage_plugin_ from dataset_service
//    storage_plugin_ = ExecEnv::GetInstance()->get_storage_plugin_();
//    storage_plugin_->ListFiles(dataset_path, &file_list);
    for (auto filepath : *file_list)
    {
      partitions->push_back(Partition(Identity(filepath, 0, 0, 0)));
    }
  // allocate location for each partition
  auto vectloc = std::make_shared<std::vector<Location>>();
  distributor->GetDistLocations(partitions);

  // build dataset
  DataSet::Data dd;
  dd.dataset_path = dataset_path;
  for (auto partt : *partitions)
    dd.partitions.push_back(partt);

  *dataset = std::make_shared<DataSet>(dd);

  return Status::OK();
}

Status DataSetBuilder::GetTotalRecords(int64_t* total_records) {
//  *total_records = file_list_->size();  //TODO: need to confirm
  return Status::OK();
}

} // namespace pegasus