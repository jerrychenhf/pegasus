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

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -ldl") 

if(NOT ARROW_FOUND)
  add_custom_target(gen-deps ALL DEPENDS boost_ep glog_ep arrow_ep)
else()
  add_custom_target(gen-deps ALL DEPENDS boost_ep glog_ep)
endif()
#add_custom_target(gen-deps ALL DEPENDS boost_ep gflags_ep)
add_custom_target(pegasus)
add_custom_target(pegasus-tests)

# Adding unit tests part of the "arrow" portion of the test suite
function(ADD_PEGASUS_TEST REL_TEST_NAME)
  set(options)
  set(one_value_args PREFIX)
  set(multi_value_args LABELS)
  cmake_parse_arguments(ARG
                        "${options}"
                        "${one_value_args}"
                        "${multi_value_args}"
                        ${ARGN})

  if(ARG_PREFIX)
    set(PREFIX ${ARG_PREFIX})
  else()
    set(PREFIX "pegasus")
  endif()

  if(ARG_LABELS)
    set(LABELS ${ARG_LABELS})
  else()
    set(LABELS "pegasus-tests")
  endif()

  add_test_case(${REL_TEST_NAME}
                PREFIX
                ${PREFIX}
                LABELS
                ${LABELS}
                ${ARG_UNPARSED_ARGUMENTS})
endfunction()

# Determine what functions are available on the current platform.
INCLUDE(CheckFunctionExists)
CHECK_FUNCTION_EXISTS(sched_getcpu HAVE_SCHED_GETCPU)

set(PEGASUS_SRCS
    vendored/base64.cpp
    vendored/datetime/tz.cpp
    vendored/double-conversion/bignum.cc
    vendored/double-conversion/double-conversion.cc
    vendored/double-conversion/bignum-dtoa.cc
    vendored/double-conversion/fast-dtoa.cc
    vendored/double-conversion/cached-powers.cc
    vendored/double-conversion/fixed-dtoa.cc
    vendored/double-conversion/diy-fp.cc
    vendored/double-conversion/strtod.cc
    vendored/uriparser/UriCommon.c
    vendored/uriparser/UriCompare.c
    vendored/uriparser/UriEscape.c
    vendored/uriparser/UriFile.c
    vendored/uriparser/UriIp4Base.c
    vendored/uriparser/UriIp4.c
    vendored/uriparser/UriMemory.c
    vendored/uriparser/UriNormalizeBase.c
    vendored/uriparser/UriNormalize.c
    vendored/uriparser/UriParseBase.c
    vendored/uriparser/UriParse.c
    vendored/uriparser/UriQuery.c
    vendored/uriparser/UriRecompose.c
    vendored/uriparser/UriResolve.c
    vendored/uriparser/UriShorten.c)

# Disable DLL exports in vendored uriparser library
add_definitions(-DURI_STATIC_BUILD)

if (NOT APPLE)
  # When compiling on Mac with clang using these linker flags are undefined and Clang on
  # Mac will abort on unknown compiler or linker flags. In the long-term we should
  # move away from using these flags to have a coherent build on OS X and Linux.
  set(WL_START_GROUP "-Wl,--start-group")
  set(WL_END_GROUP "-Wl,--end-group")
endif()

# resolve "#include "<subdir>/<name>.h"
include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR})
include_directories(BEFORE ${CMAKE_CURRENT_BINARY_DIR})

set (PEGASUS_LIBS
  pegasus_ipc
  cache
  catalog
  common
  dataset
  parquet
  runtime
  planner
  worker
  storage
  util
  gutil
)

set (PEGASUS_LINK_LIBS
  ${WL_START_GROUP}
  ${PEGASUS_LIBS}
  ${WL_END_GROUP}
)
add_subdirectory(gutil)
add_subdirectory(rpc)
add_subdirectory(util)
add_subdirectory(common)
add_subdirectory(catalog)
add_subdirectory(parquet)
add_subdirectory(cache)
add_subdirectory(client)
add_subdirectory(dataset)
add_subdirectory(storage)
add_subdirectory(runtime)
add_subdirectory(ipc)
add_subdirectory(server/planner)
add_subdirectory(server/worker)

if(NOT APPLE AND NOT MSVC)
  # Localize thirdparty symbols using a linker version script. This hides them
  # from the client application. The OS X linker does not support the
  # version-script option.
  set(PEGASUS_VERSION_SCRIPT_FLAGS
      "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/symbols.map")
  set(PEGASUS_SHARED_LINK_FLAGS ${PEGASUS_VERSION_SCRIPT_FLAGS})
endif()

set(PEGASUS_ALL_SRCS ${PEGASUS_SRCS})

add_pegasus_lib(pegasus
              CMAKE_PACKAGE_NAME
              Pegasus
              PKG_CONFIG_NAME
              pegasus
              SOURCES
              ${PEGASUS_ALL_SRCS}
              OUTPUTS
              PEGASUS_LIBRARIES
              DEPENDENCIES
              pegasus_dependencies
              SHARED_LINK_FLAGS
              ${PEGASUS_SHARED_LINK_FLAGS}
              SHARED_LINK_LIBS
              ${PEGASUS_LINK_LIBS}
              SHARED_PRIVATE_LINK_LIBS
              ${PEGASUS_SHARED_PRIVATE_LINK_LIBS}
              STATIC_LINK_LIBS
              ${PEGASUS_STATIC_LINK_LIBS}
              SHARED_INSTALL_INTERFACE_LIBS
              ${PEGASUS_SHARED_INSTALL_INTERFACE_LIBS}
              STATIC_INSTALL_INTERFACE_LIBS
              ${PEGASUS_STATIC_INSTALL_INTERFACE_LIBS})

add_dependencies(pegasus ${PEGASUS_LIBRARIES})

if(PEGASUS_BUILD_STATIC AND WIN32)
  target_compile_definitions(pegasus_static PUBLIC PEGASUS_STATIC)
endif()

if(PEGASUS_WITH_BACKTRACE)
  find_package(Backtrace)

  foreach(LIB_TARGET ${PEGASUS_LIBRARIES})
    target_compile_definitions(${LIB_TARGET} PRIVATE PEGASUS_EXPORTING)
    if(Backtrace_FOUND AND PEGASUS_WITH_BACKTRACE)
      target_compile_definitions(${LIB_TARGET} PRIVATE PEGASUS_WITH_BACKTRACE)
    endif()
  endforeach()
endif()

pegasus_install_all_headers("pegasus")

config_summary_cmake_setters("${CMAKE_CURRENT_BINARY_DIR}/PegasusOptions.cmake")
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/PegasusOptions.cmake
        DESTINATION "${PEGASUS_CMAKE_INSTALL_DIR}")

# For backward compatibility for find_package(arrow)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/pegasus-config.cmake
        DESTINATION "${PEGASUS_CMAKE_INSTALL_DIR}")


if(PEGASUS_RPC_JAVA_CLIENT_JNI)
  # rpc java client jni support
  find_package(JNI REQUIRED)
  # add jni support
  include_directories(${JAVA_INCLUDE_PATH})
  include_directories(${JAVA_INCLUDE_PATH2})
  if(JNI_FOUND)
    message(STATUS "JNI_INCLUDE_DIRS = ${JNI_INCLUDE_DIRS}")
    message(STATUS "JNI_LIBRARIES = ${JNI_LIBRARIES}")
  else()
    message(WARNING "Could not find JNI")
  endif()

  add_compile_options("-I$ENV{JAVA_HOME}/include/")
  if(WIN32)
    add_compile_options("-I$ENV{JAVA_HOME}/include/win32")
  elseif(APPLE)
    add_compile_options("-I$ENV{JAVA_HOME}/include/darwin")
  else() # linux
    add_compile_options("-I$ENV{JAVA_HOME}/include/linux")
  endif()

  include_directories("${CMAKE_CURRENT_LIST_DIR}/java")

  file(GLOB_RECURSE PEGASUS_RPC_JAVA_CLIENT_JNI_SRC java/*.cc)
  add_library(pegasus_java SHARED ${PEGASUS_RPC_JAVA_CLIENT_JNI_SRC})

  if(APPLE)
    target_link_libraries(pegasus_java
                          pegasus_ipc
                          "-undefined dynamic_lookup"
                          ${PTHREAD_LIBRARY})
  else(APPLE)
    target_link_libraries(pegasus_java
                          pegasus_ipc
                          ${PTHREAD_LIBRARY})
  endif(APPLE)
endif()


