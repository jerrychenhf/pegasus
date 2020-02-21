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

add_library(DataSet
  filter.cc
  partition.cc
  identity.cc
  consistent_hashing.cc
  dataset_listing.cc
  dataset_request.cc
  dataset_service.cc
  dataset_store.cc
  dataset.cc
  dataset_builder.cc
  flightinfo_builder.cc
  dataset_cache_manager.cc
  dataset_cache_block_manager.cc
  cache_engine.cc
  cache_store.cc
  cache_store_manager.cc
  dataset_cache_engine_manager.cc
  request_identity.cc
)
add_dependencies(DataSet gen-deps)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -std=c++0x")

if(PEGASUS_TEST_LINKAGE STREQUAL "static")
  set(PEGASUS_DATASET_TEST_LINK_LIBS
      DataSet
      ${PARQUET_SHARED_LIB}
      pegasus_rpc_shared
      ${ARROW_STATIC_LIB}
      ${ARROW_TESTING_SHARED_LIB}
      ${PEGASUS_TEST_STATIC_LINK_LIBS})
else()
  set(PEGASUS_DATASET_TEST_LINK_LIBS
      DataSet
      ${PARQUET_SHARED_LIB}
      pegasus_rpc_shared
      ${ARROW_SHARED_LIB}
      ${ARROW_TESTING_SHARED_LIB}
      ${PEGASUS_TEST_SHARED_LINK_LIBS})
endif()

# Adding unit tests part of the "util" portion of the test suite
function(ADD_PEGASUS_DATASET_TEST REL_TEST_NAME)
  set(options --allow-shlib-undefined)
  set(one_value_args PREFIX)
  set(multi_value_args LABELS)
  cmake_parse_arguments(ARG
                        "${options}"
                        "${one_value_args}"
                        "${multi_value_args}"
                        ${ARGN})

  add_pegasus_test(${REL_TEST_NAME}
                 EXTRA_LINK_LIBS
                 ${PEGASUS_DATASET_TEST_LINK_LIBS})
endfunction()

if(PEGASUS_BUILD_TESTS)

  add_pegasus_dataset_test(dataset_cache_manager_test)
  add_pegasus_dataset_test(dataset_service_test)

endif()


