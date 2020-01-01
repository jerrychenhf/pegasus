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

message (STATUS "Common ${BUILD_OUTPUT_ROOT_DIRECTORY}")
# where to put generated libraries
set(LIBRARY_OUTPUT_PATH "${BUILD_OUTPUT_ROOT_DIRECTORY}/dataset")

# where to put generated binaries
set(EXECUTABLE_OUTPUT_PATH "${BUILD_OUTPUT_ROOT_DIRECTORY}/dataset")

add_library(Endpoint
  endpoint.cc
  identity.cc
)
add_dependencies(Endpoint gen-deps)

add_library(Dataset
  dataset_listing.cc
  dataset_service.cc
  dataset_store.cc
  dataset.cc
  dataset_builder.cc
)
add_dependencies(Dataset gen-deps)

add_library(FlightInfo
  flightinfo_builder.cc
)
add_dependencies(FlightInfo gen-deps)

#target_link_libraries(Dataset Status SparkCatalog Runtime Location FlightInfo Endpoint)

