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
set(LIBRARY_OUTPUT_PATH "${BUILD_OUTPUT_ROOT_DIRECTORY}/storage")

# where to put generated binaries
set(EXECUTABLE_OUTPUT_PATH "${BUILD_OUTPUT_ROOT_DIRECTORY}/storage")

add_library(StoragePlugin
  storage_plugin.cc
  storage_plugin_factory.cc
)
add_dependencies(StoragePlugin gen-deps)

set(PEGASUS_STORAGE_TEST_LINK_LIBS
    StoragePlugin
    Common
    Util
    Parquet)

if(PEGASUS_TEST_LINKAGE STREQUAL "static")
  list(APPEND PEGASUS_STORAGE_TEST_LINK_LIBS
      pegasus_rpc_static
      ${ARROW_STATIC_LIB}
      ${PEGASUS_TEST_STATIC_LINK_LIBS})
else()
  list(APPEND PEGASUS_STORAGE_TEST_LINK_LIBS
      pegasus_rpc_shared
      ${ARROW_SHARED_LIB}
      ${PEGASUS_TEST_SHARED_LINK_LIBS})
endif()

# Adding unit tests part of the "storage" portion of the test suite
function(ADD_PEGASUS_STORAGE_TEST REL_TEST_NAME)
  set(options)
  set(one_value_args PREFIX)
  set(multi_value_args LABELS)
  cmake_parse_arguments(ARG
                        "${options}"
                        "${one_value_args}"
                        "${multi_value_args}"
                        ${ARGN})

  add_pegasus_test(${REL_TEST_NAME}
                 EXTRA_LINK_LIBS
                 ${PEGASUS_STORAGE_TEST_LINK_LIBS})
endfunction()

if(PEGASUS_BUILD_TESTS)

  add_pegasus_storage_test(storage_plugin_test)

endif()


