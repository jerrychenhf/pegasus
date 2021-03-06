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

#ifndef PEGASUS_DATASET_STORE_H
#define PEGASUS_DATASET_STORE_H

#include <unordered_map>
#include <atomic>
#include <boost/thread/locks.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <util/spinlock.h>
#include "common/status.h"
#include "dataset/dataset.h"

namespace pegasus {

class DataSetStore {
 public:
  DataSetStore();
  ~DataSetStore();
  
  Status GetDataSets(std::shared_ptr<std::vector<std::shared_ptr<DataSet>>>* datasets);
  Status GetDataSet(std::string dataset_path, std::shared_ptr<DataSet>* dataset);
  Status InsertDataSet(std::shared_ptr<DataSet> dataset);
  Status InvalidateAll();
  Status ReplacePartitions(std::string& datasetpath, std::shared_ptr<std::vector<Partition>> partits);
  Status UpdateDataSet(std::shared_ptr<DataSet> dataset);
  Status RemoveDataSet(std::shared_ptr<DataSet> dataset);

 private:
  SpinLock wholestore_lock_;
  std::unordered_map<std::string, std::shared_ptr<DataSet>> dataset_map_;
};

} // namespace pegasus

#endif  // PEGASUS_DATASET_STORE_H