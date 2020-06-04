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

#ifndef PEGASUS_FILE_BATCH_H
#define PEGASUS_FILE_BATCH_H

#include "common/status.h"

using namespace std;
namespace arrow {
  class Buffer;
}

namespace pegasus {

class FileBatch {
 public:
 	FileBatch(int rowgroup_id, std::vector<std::shared_ptr<arrow::Buffer>> object_buffers):
	 rowgroup_id_(rowgroup_id), object_buffers_(object_buffers) {}
 	
 	int rowgroup_id() {return rowgroup_id_;}
	std::vector<std::shared_ptr<arrow::Buffer>> object_buffers() {return object_buffers_;}
 private:
  int rowgroup_id_;
  std::vector<std::shared_ptr<arrow::Buffer>> object_buffers_;
};

} // namespace pegasus

#endif  // PEGASUS_FILE_BATCH_H