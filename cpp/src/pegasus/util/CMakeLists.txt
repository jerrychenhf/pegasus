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


message (STATUS "Util ${BUILD_OUTPUT_ROOT_DIRECTORY}")
# where to put generated libraries
set(LIBRARY_OUTPUT_PATH "${BUILD_OUTPUT_ROOT_DIRECTORY}/util")

# where to put generated binaries
set(EXECUTABLE_OUTPUT_PATH "${BUILD_OUTPUT_ROOT_DIRECTORY}/util")

add_library(Util
  consistent_hashing.cc
  conhash.c
  conhash_inter.c
  conhash_util.c
  util_rbtree.c
  md5.c
  global_flags.cc
  string_builder.cc
  time.cc
  os-info.cc
  thread.cc
  error-util.cc
  network-util.cc
  test-info.cc
  cpu-info.cc
  disk-info.cc
  mem-info.cc
  debug-util.cc
)
add_dependencies(Util gen-deps)

target_link_libraries(Util
                      ${BOOST_STATIC_FILESYSTEM_LIBRARY}
                      ${GFLAGS_LIBRARIES})

if(PEGASUS_TEST_LINKAGE STREQUAL "static")
  set(PEGASUS_UTIL_TEST_LINK_LIBS 
      Util
      ${PEGASUS_TEST_STATIC_LINK_LIBS})
else()
  set(PEGASUS_UTIL_TEST_LINK_LIBS
      Util
      ${PEGASUS_TEST_SHARED_LINK_LIBS})
endif()

# Adding unit tests part of the "util" portion of the test suite
function(ADD_PEGASUS_UTIL_TEST REL_TEST_NAME)
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
                 ${PEGASUS_UTIL_TEST_LINK_LIBS})
endfunction()

if(PEGASUS_BUILD_TESTS)

  add_pegasus_util_test(thread-pool-test)

  add_pegasus_util_test(pretty-printer-test)

endif()