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

#ifndef PEGASUS_DATASET_H
#define PEGASUS_DATASET_H

#include <string>
#include <vector>

#include <arrow/type.h>

#include <boost/thread/shared_mutex.hpp>

#include "dataset/partition.h"
#include "util/visibility.h"

namespace pegasus {

class rwlock
{
public:
    rwlock() 
        : readCount(0), 
          writeCount(0) 
    {
    }

    void lockread()
    {   
        internal.lock();
        if (readCount == 0)
        {   
            content.lock();
        }
        ++readCount;
        assert(readCount > 0);
        assert(writeCount == 0);
//        std::cout << "readCount = " << readCount << "\n";
        internal.unlock();
    }
    
    void lockwrite()
    {   
        content.lock();
        ++writeCount;
        assert(writeCount == 1);
        assert(readCount == 0);
//        std::cout << "writeCount = " << writeCount << "\n";
    }

    void unlockread()
    {   
        internal.lock();
        --readCount;
        assert(readCount >= 0);
        assert(writeCount == 0);
//        std::cout << "readCount = " << readCount << "\n";
        if (readCount == 0)
        {   
            content.unlock();
        }
        internal.unlock();
    }

    void unlockwrite()
    {   
        --writeCount;
        assert(writeCount == 0);
        assert(readCount == 0);
//        std::cout << "writeCount = " << writeCount << "\n";
        content.unlock();
    }

private:
    boost::mutex content;
    boost::mutex internal;
    int readCount;
    int writeCount;
};

/// \brief The access coordinates for retireval of a dataset
class PEGASUS_EXPORT DataSet {
 public:
  struct Data {
    /// Path identifying a particular dataset. 
    std::string dataset_path;
    std::vector<Partition> partitions;
    uint64_t timestamp;
    int64_t total_records;
    int64_t total_bytes;
  };

  explicit DataSet(const Data& data) : data_(data) {}
  explicit DataSet(Data&& data)
      : data_(std::move(data)) {}

  /// Get the data_
  const Data& GetData() {return data_;}

  std::shared_ptr<arrow::Schema> get_schema() {
    return schema_;
  }

  void set_schema(std::shared_ptr<arrow::Schema> schema) {
    schema_ = schema;
  }

  /// The path of the dataset
  const std::string& dataset_path() const { return data_.dataset_path; }

  /// A list of partitions associated with the dataset.
  const std::vector<Partition>& partitions() const { return data_.partitions; }

  /// The total number of records (rows) in the dataset. If unknown, set to -1
  int64_t total_records() const { return data_.total_records; }

  /// The total number of bytes in the dataset. If unknown, set to -1
  int64_t total_bytes() const { return data_.total_bytes; }

  void lockread() { dslock.lockread(); }
  void unlockread() { dslock.unlockread(); }
  void lockwrite() { dslock.lockwrite(); }
  void unlockwrite() { dslock.unlockwrite(); }

 private:
  rwlock dslock;
  Data data_;
  std::shared_ptr<arrow::Schema> schema_;
};

class PEGASUS_EXPORT ResultDataSet {
 public:
  struct Data {
    // arrow::Schema schema;
    /// Path identifying a particular dataset. 
    std::string dataset_path;
    std::vector<Partition> partitions;
    int64_t total_records;
    int64_t total_bytes;
  };

  explicit ResultDataSet(const Data& data) : data_(data) {}
  explicit ResultDataSet(Data&& data)
      : data_(std::move(data)) {}

  std::shared_ptr<arrow::Schema> get_schema() {
    return schema_;
  }

  void set_schema(std::shared_ptr<arrow::Schema> schema) {
    schema_ = schema;
  }

  /// The path of the dataset
  const std::string& dataset_path() const { return data_.dataset_path; }

  /// A list of partitions associated with the dataset.
  const std::vector<Partition>& partitions() const { return data_.partitions; }

  /// The total number of records (rows) in the dataset. If unknown, set to -1
  int64_t total_records() const { return data_.total_records; }

  /// The total number of bytes in the dataset. If unknown, set to -1
  int64_t total_bytes() const { return data_.total_bytes; }

 private:
  Data data_;
  std::shared_ptr<arrow::Schema> schema_;
};

} // namespace pegasus

#endif  // PEGASUS_DATASET_H