# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.


add_custom_target(gen-deps)

if (NOT APPLE)
  # When compiling on Mac with clang using these linker flags are undefined and Clang on
  # Mac will abort on unknown compiler or linker flags. In the long-term we should
  # move away from using these flags to have a coherent build on OS X and Linux.
  set(WL_START_GROUP "-Wl,--start-group")
  set(WL_END_GROUP "-Wl,--end-group")
endif()

set (PEGASUS_LIBS
  CacheManager
  Store
  SparkCatalog
  Status
  Location
  WorkerManager
  Partition
  Dataset
  FlightInfo
  Parquet
  Runtime
  Planner
  Worker
  StoragePlugin
  Hash
  GlobalFlags
  Log
  StringBuilder
)

set (PEGASUS_LINK_LIBS
  ${WL_START_GROUP}
  ${PEGASUS_LIBS}
  ${WL_END_GROUP}
)

add_subdirectory(util)
add_subdirectory(common)
add_subdirectory(catalog)
add_subdirectory(parquet)
add_subdirectory(cache)
add_subdirectory(client)
add_subdirectory(dataset)
add_subdirectory(storage)
add_subdirectory(runtime)
add_subdirectory(server/planner)
add_subdirectory(server/worker)




