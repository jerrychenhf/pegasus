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

# where to put generated libraries
set(LIBRARY_OUTPUT_PATH "${BUILD_OUTPUT_ROOT_DIRECTORY}/worker")

# where to put generated binaries
set(EXECUTABLE_OUTPUT_PATH "${BUILD_OUTPUT_ROOT_DIRECTORY}/worker")

add_library(Worker
  worker_table_api_service.cc
  worker.cc
)
add_dependencies(Worker gen-deps)

add_executable(workerd worker_main.cc)
target_link_libraries(workerd
  pegasus_rpc_shared
  ${PEGASUS_LINK_LIBS}
  ${ARROW_SHARED_LIB}
  ${PARQUET_SHARED_LIB}
  ${GFLAGS_LIBRARIES}
  ${BOOST_SYSTEM_LIBRARY}
  ${BOOST_STATIC_FILESYSTEM_LIBRARY}
  ${GLOG_STATIC_LIB}
)


