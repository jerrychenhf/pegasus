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

#ifndef PEGASUS_PROPERTIES_H_
#define PEGASUS_PROPERTIES_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "arrow/type.h"
#include "arrow/util/compression.h"
#include "parquet/encryption.h"
#include "parquet/exception.h"
#include "parquet/parquet_version.h"
#include "parquet/platform.h"
#include "parquet/schema.h"
#include "parquet/types.h"

namespace pegasus {

DEFINE_string(kudu_read_mode, "READ_LATEST", "(Advanced) Sets the Kudu scan ReadMode. "
    "Supported Kudu read modes are READ_LATEST and READ_AT_SNAPSHOT. Can be overridden "
    "with the query option of the same name.");

}  // namespace pegasus

#endif  // PEGASUS_PROPERTIES_H_
