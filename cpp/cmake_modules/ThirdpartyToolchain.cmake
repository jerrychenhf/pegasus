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

include(ProcessorCount)
processorcount(NPROC)

add_custom_target(rapidjson)
add_custom_target(toolchain)
add_custom_target(toolchain-benchmarks)
add_custom_target(toolchain-tests)

# ----------------------------------------------------------------------
# Toolchain linkage options

set(PEGASUS_RE2_LINKAGE
    "static"
    CACHE STRING "How to link the re2 library. static|shared (default static)")

if(PEGASUS_PROTOBUF_USE_SHARED)
  set(Protobuf_USE_STATIC_LIBS OFF)
else()
  set(Protobuf_USE_STATIC_LIBS ON)
endif()

# ----------------------------------------------------------------------
# We should not use the Apache dist server for build dependencies

set(APACHE_MIRROR "")

macro(get_apache_mirror)
  if(APACHE_MIRROR STREQUAL "")
    execute_process(COMMAND ${PYTHON_EXECUTABLE}
                            ${CMAKE_SOURCE_DIR}/build-support/get_apache_mirror.py
                    OUTPUT_VARIABLE APACHE_MIRROR
                    OUTPUT_STRIP_TRAILING_WHITESPACE)
  endif()
endmacro()

# ----------------------------------------------------------------------
# Resolve the dependencies

set(PEGASUS_THIRDPARTY_DEPENDENCIES
    BOOST
    gflags
    GLOG
    gRPC
    GTest
    LLVM
    Lz4
    ORC
    RE2
    Protobuf
    Snappy
    Thrift
    uriparser
    Arrow)

# TODO(wesm): External GTest shared libraries are not currently
# supported when building with MSVC because of the way that
# conda-forge packages have 4 variants of the libraries packaged
# together
if(MSVC AND "${GTest_SOURCE}" STREQUAL "")
  set(GTest_SOURCE "BUNDLED")
endif()

message(STATUS "Using ${PEGASUS_DEPENDENCY_SOURCE} approach to find dependencies")

if(PEGASUS_DEPENDENCY_SOURCE STREQUAL "CONDA")
  if(MSVC)
    set(PEGASUS_PACKAGE_PREFIX "$ENV{CONDA_PREFIX}/Library")
  else()
    set(PEGASUS_PACKAGE_PREFIX $ENV{CONDA_PREFIX})
  endif()
  set(PEGASUS_ACTUAL_DEPENDENCY_SOURCE "SYSTEM")
  message(STATUS "Using CONDA_PREFIX for PEGASUS_PACKAGE_PREFIX: ${PEGASUS_PACKAGE_PREFIX}")
else()
  set(PEGASUS_ACTUAL_DEPENDENCY_SOURCE "${PEGASUS_DEPENDENCY_SOURCE}")
endif()

if(PEGASUS_PACKAGE_PREFIX)
  message(STATUS "Setting (unset) dependency *_ROOT variables: ${PEGASUS_PACKAGE_PREFIX}")
  set(ENV{PKG_CONFIG_PATH} "${PEGASUS_PACKAGE_PREFIX}/lib/pkgconfig/")

  if(NOT ENV{BOOST_ROOT})
    set(ENV{BOOST_ROOT} ${PEGASUS_PACKAGE_PREFIX})
  endif()
  if(NOT ENV{Boost_ROOT})
    set(ENV{Boost_ROOT} ${PEGASUS_PACKAGE_PREFIX})
  endif()
endif()

# For each dependency, set dependency source to global default, if unset
foreach(DEPENDENCY ${PEGASUS_THIRDPARTY_DEPENDENCIES})
  if("${${DEPENDENCY}_SOURCE}" STREQUAL "")
    set(${DEPENDENCY}_SOURCE ${PEGASUS_ACTUAL_DEPENDENCY_SOURCE})
    # If no ROOT was supplied and we have a global prefix, use it
    if(NOT ${DEPENDENCY}_ROOT AND PEGASUS_PACKAGE_PREFIX)
      set(${DEPENDENCY}_ROOT ${PEGASUS_PACKAGE_PREFIX})
    endif()
  endif()
endforeach()

macro(build_dependency DEPENDENCY_NAME)
  if("${DEPENDENCY_NAME}" STREQUAL "BZip2")
    build_bzip2()
  elseif("${DEPENDENCY_NAME}" STREQUAL "gflags")
    build_gflags()
  elseif("${DEPENDENCY_NAME}" STREQUAL "GLOG")
    build_glog()
  elseif("${DEPENDENCY_NAME}" STREQUAL "gRPC")
    build_grpc()
  elseif("${DEPENDENCY_NAME}" STREQUAL "GTest")
    build_gtest()
  elseif("${DEPENDENCY_NAME}" STREQUAL "Protobuf")
    build_protobuf()
  elseif("${DEPENDENCY_NAME}" STREQUAL "Thrift")
    build_thrift()
  elseif("${DEPENDENCY_NAME}" STREQUAL "uriparser")
    build_uriparser()
  elseif("${DEPENDENCY_NAME}" STREQUAL "Arrow")
    build_arrow()
  else()
    message(FATAL_ERROR "Unknown thirdparty dependency to build: ${DEPENDENCY_NAME}")
  endif()
endmacro()

macro(resolve_dependency DEPENDENCY_NAME)
  if(${DEPENDENCY_NAME}_SOURCE STREQUAL "AUTO")
    find_package(${DEPENDENCY_NAME} MODULE)
    if(NOT ${${DEPENDENCY_NAME}_FOUND})
      build_dependency(${DEPENDENCY_NAME})
    endif()
  elseif(${DEPENDENCY_NAME}_SOURCE STREQUAL "BUNDLED")
    build_dependency(${DEPENDENCY_NAME})
  elseif(${DEPENDENCY_NAME}_SOURCE STREQUAL "SYSTEM")
    find_package(${DEPENDENCY_NAME} REQUIRED)
  endif()
endmacro()

macro(resolve_dependency_with_version DEPENDENCY_NAME REQUIRED_VERSION)
  if(${DEPENDENCY_NAME}_SOURCE STREQUAL "AUTO")
    find_package(${DEPENDENCY_NAME} ${REQUIRED_VERSION} MODULE)
    if(NOT ${${DEPENDENCY_NAME}_FOUND})
      build_dependency(${DEPENDENCY_NAME})
    endif()
  elseif(${DEPENDENCY_NAME}_SOURCE STREQUAL "BUNDLED")
    build_dependency(${DEPENDENCY_NAME})
  elseif(${DEPENDENCY_NAME}_SOURCE STREQUAL "SYSTEM")
    find_package(${DEPENDENCY_NAME} ${REQUIRED_VERSION} REQUIRED)
  endif()
endmacro()

# ----------------------------------------------------------------------
# Thirdparty versions, environment variables, source URLs

set(THIRDPARTY_DIR "${pegasus_SOURCE_DIR}/thirdparty")

# Include vendored Flatbuffers
include_directories(SYSTEM "${THIRDPARTY_DIR}/flatbuffers/include")

# ----------------------------------------------------------------------
# Some EP's require other EP's
  set(PEGASUS_WITH_THRIFT OFF)
  set(PEGASUS_WITH_GRPC OFF)
  set(PEGASUS_WITH_URIPARSER OFF)
  set(PEGASUS_WITH_PROTOBUF OFF)


# ----------------------------------------------------------------------
# Versions and URLs for toolchain builds, which also can be used to configure
# offline builds

# Read toolchain versions from cpp/thirdparty/versions.txt
file(STRINGS "${THIRDPARTY_DIR}/versions.txt" TOOLCHAIN_VERSIONS_TXT)
foreach(_VERSION_ENTRY ${TOOLCHAIN_VERSIONS_TXT})
  # Exclude comments
  if(NOT
     ((_VERSION_ENTRY MATCHES "^[^#][A-Za-z0-9-_]+_VERSION=")
      OR (_VERSION_ENTRY MATCHES "^[^#][A-Za-z0-9-_]+_CHECKSUM=")))
    continue()
  endif()

  string(REGEX MATCH "^[^=]*" _LIB_NAME ${_VERSION_ENTRY})
  string(REPLACE "${_LIB_NAME}=" "" _LIB_VERSION ${_VERSION_ENTRY})

  # Skip blank or malformed lines
  if(${_LIB_VERSION} STREQUAL "")
    continue()
  endif()

  # For debugging
  message(STATUS "${_LIB_NAME}: ${_LIB_VERSION}")

  set(${_LIB_NAME} "${_LIB_VERSION}")
endforeach()

if(DEFINED ENV{PEGASUS_BOOST_URL})
  set(BOOST_SOURCE_URL "$ENV{PEGASUS_BOOST_URL}")
else()
  string(REPLACE "." "_" BOOST_VERSION_UNDERSCORES ${BOOST_VERSION})
  set(
    BOOST_SOURCE_URL
    "https://dl.bintray.com/boostorg/release/${BOOST_VERSION}/source/boost_${BOOST_VERSION_UNDERSCORES}.tar.gz"
    )
endif()


if(DEFINED ENV{PEGASUS_GFLAGS_URL})
  set(GFLAGS_SOURCE_URL "$ENV{PEGASUS_GFLAGS_URL}")
else()
  set(GFLAGS_SOURCE_URL
      "https://github.com/gflags/gflags/archive/${GFLAGS_VERSION}.tar.gz")
endif()

if(DEFINED ENV{PEGASUS_GLOG_URL})
  set(GLOG_SOURCE_URL "$ENV{PEGASUS_GLOG_URL}")
else()
  set(GLOG_SOURCE_URL "https://github.com/google/glog/archive/${GLOG_VERSION}.tar.gz")
endif()

if(DEFINED ENV{PEGASUS_GRPC_URL})
  set(GRPC_SOURCE_URL "$ENV{PEGASUS_GRPC_URL}")
else()
  set(GRPC_SOURCE_URL "https://github.com/grpc/grpc/archive/${GRPC_VERSION}.tar.gz")
endif()

if(DEFINED ENV{PEGASUS_GTEST_URL})
  set(GTEST_SOURCE_URL "$ENV{PEGASUS_GTEST_URL}")
else()
  set(GTEST_SOURCE_URL
      "https://github.com/google/googletest/archive/release-${GTEST_VERSION}.tar.gz")
endif()

if(DEFINED ENV{PEGASUS_JEMALLOC_URL})
  set(JEMALLOC_SOURCE_URL "$ENV{PEGASUS_JEMALLOC_URL}")
else()
  set(
    JEMALLOC_SOURCE_URL
    "https://github.com/jemalloc/jemalloc/releases/download/${JEMALLOC_VERSION}/jemalloc-${JEMALLOC_VERSION}.tar.bz2"
    )
endif()

if(DEFINED ENV{PEGASUS_MIMALLOC_URL})
  set(MIMALLOC_SOURCE_URL "$ENV{PEGASUS_MIMALLOC_URL}")
else()
  set(MIMALLOC_SOURCE_URL
      "https://github.com/microsoft/mimalloc/archive/${MIMALLOC_VERSION}.tar.gz")
endif()

if(DEFINED ENV{PEGASUS_PROTOBUF_URL})
  set(PROTOBUF_SOURCE_URL "$ENV{PEGASUS_PROTOBUF_URL}")
else()
  string(SUBSTRING ${PROTOBUF_VERSION} 1 -1 STRIPPED_PROTOBUF_VERSION)
  # strip the leading `v`
  set(
    PROTOBUF_SOURCE_URL
    "https://github.com/protocolbuffers/protobuf/releases/download/${PROTOBUF_VERSION}/protobuf-all-${STRIPPED_PROTOBUF_VERSION}.tar.gz"
    )
endif()

if(DEFINED ENV{PEGASUS_SNAPPY_URL})
  set(SNAPPY_SOURCE_URL "$ENV{PEGASUS_SNAPPY_URL}")
else()
  set(SNAPPY_SOURCE_URL
      "https://github.com/google/snappy/archive/${SNAPPY_VERSION}.tar.gz")
endif()

if(DEFINED ENV{PEGASUS_THRIFT_URL})
  set(THRIFT_SOURCE_URL "$ENV{PEGASUS_THRIFT_URL}")
else()
  set(THRIFT_SOURCE_URL "FROM-APACHE-MIRROR")
endif()

if(DEFINED ENV{PEGASUS_URIPARSER_URL})
  set(URIPARSER_SOURCE_URL "$ENV{PEGASUS_URIPARSER_URL}")
else()
  set(
    URIPARSER_SOURCE_URL

    "https://github.com/uriparser/uriparser/archive/uriparser-${URIPARSER_VERSION}.tar.gz"
    )
endif()

# ----------------------------------------------------------------------
# ExternalProject options

set(EP_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_${UPPERCASE_BUILD_TYPE}}")
set(EP_C_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_${UPPERCASE_BUILD_TYPE}}")

if(NOT MSVC)
  # Set -fPIC on all external projects
  set(EP_CXX_FLAGS "${EP_CXX_FLAGS} -fPIC")
  set(EP_C_FLAGS "${EP_C_FLAGS} -fPIC")
endif()

# CC/CXX environment variables are captured on the first invocation of the
# builder (e.g make or ninja) instead of when CMake is invoked into to build
# directory. This leads to issues if the variables are exported in a subshell
# and the invocation of make/ninja is in distinct subshell without the same
# environment (CC/CXX).
set(EP_COMMON_TOOLCHAIN -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
                        -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER})

if(CMAKE_AR)
  set(EP_COMMON_TOOLCHAIN ${EP_COMMON_TOOLCHAIN} -DCMAKE_AR=${CMAKE_AR})
endif()

if(CMAKE_RANLIB)
  set(EP_COMMON_TOOLCHAIN ${EP_COMMON_TOOLCHAIN} -DCMAKE_RANLIB=${CMAKE_RANLIB})
endif()

# External projects are still able to override the following declarations.
# cmake command line will favor the last defined variable when a duplicate is
# encountered. This requires that `EP_COMMON_CMAKE_ARGS` is always the first
# argument.
set(EP_COMMON_CMAKE_ARGS
    ${EP_COMMON_TOOLCHAIN}
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    -DCMAKE_C_FLAGS=${EP_C_FLAGS}
    -DCMAKE_C_FLAGS_${UPPERCASE_BUILD_TYPE}=${EP_C_FLAGS}
    -DCMAKE_CXX_FLAGS=${EP_CXX_FLAGS}
    -DCMAKE_CXX_FLAGS_${UPPERCASE_BUILD_TYPE}=${EP_CXX_FLAGS})

set(EP_LOG_OPTIONS
      LOG_CONFIGURE
      1
      LOG_BUILD
      1
      LOG_INSTALL
      1
      LOG_DOWNLOAD
      1)
set(Boost_DEBUG FALSE)

# Ensure that a default make is set
if("${MAKE}" STREQUAL "")
  if(NOT MSVC)
    find_program(MAKE make)
  endif()
endif()

# Using make -j in sub-make is fragile
if(${CMAKE_GENERATOR} MATCHES "Makefiles")
  set(MAKE_BUILD_ARGS "")
else()
  # limit the maximum number of jobs for ninja
  set(MAKE_BUILD_ARGS "-j${NPROC}")
endif()

# ----------------------------------------------------------------------
# Find pthreads

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# ----------------------------------------------------------------------
# Add Boost dependencies (code adapted from Apache Kudu)

macro(build_boost)
  set(BOOST_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/boost_ep-prefix/src/boost_ep")

  # This is needed by the thrift_ep build
  set(BOOST_ROOT ${BOOST_PREFIX})

  set(BOOST_LIB_DIR "${BOOST_PREFIX}/stage/lib")
  set(BOOST_BUILD_LINK "static")
  if(MSVC)
    string(REGEX
           REPLACE "^([0-9]+)\\.([0-9]+)\\.[0-9]+$" "\\1_\\2"
                   BOOST_VERSION_NO_MICRO_UNDERSCORE ${BOOST_VERSION})
    set(BOOST_LIBRARY_SUFFIX
        "-vc${MSVC_TOOLSET_VERSION}-mt-x64-${BOOST_VERSION_NO_MICRO_UNDERSCORE}")
  else()
    set(BOOST_LIBRARY_SUFFIX "")
  endif()
  set(
    BOOST_STATIC_SYSTEM_LIBRARY
    "${BOOST_LIB_DIR}/libboost_system${BOOST_LIBRARY_SUFFIX}${CMAKE_STATIC_LIBRARY_SUFFIX}"
    )
  set(
    BOOST_STATIC_FILESYSTEM_LIBRARY
    "${BOOST_LIB_DIR}/libboost_filesystem${BOOST_LIBRARY_SUFFIX}${CMAKE_STATIC_LIBRARY_SUFFIX}"
    )
  set(
    BOOST_STATIC_REGEX_LIBRARY

    "${BOOST_LIB_DIR}/libboost_regex${BOOST_LIBRARY_SUFFIX}${CMAKE_STATIC_LIBRARY_SUFFIX}"
    )
  set(BOOST_SYSTEM_LIBRARY boost_system_static)
  set(BOOST_FILESYSTEM_LIBRARY boost_filesystem_static)
  set(BOOST_REGEX_LIBRARY boost_regex_static)
  set(BOOST_BUILD_PRODUCTS ${BOOST_STATIC_SYSTEM_LIBRARY}
                           ${BOOST_STATIC_FILESYSTEM_LIBRARY}
                           ${BOOST_STATIC_REGEX_LIBRARY})
  if(MSVC)
    set(BOOST_CONFIGURE_COMMAND ".\\\\bootstrap.bat")
  else()
    set(BOOST_CONFIGURE_COMMAND "./bootstrap.sh")
  endif()
  list(APPEND BOOST_CONFIGURE_COMMAND "--prefix=${BOOST_PREFIX}"
              "--with-libraries=filesystem,regex,system")
  if("${CMAKE_BUILD_TYPE}" STREQUAL "DEBUG")
    set(BOOST_BUILD_VARIANT "debug")
  else()
    set(BOOST_BUILD_VARIANT "release")
  endif()
  set(BOOST_BUILD_COMMAND "./b2" "-j${NPROC}" "link=${BOOST_BUILD_LINK}"
                          "variant=${BOOST_BUILD_VARIANT}")
  if(MSVC)
    string(REGEX
           REPLACE "([0-9])$" ".\\1" BOOST_TOOLSET_MSVC_VERSION ${MSVC_TOOLSET_VERSION})
    list(APPEND BOOST_BUILD_COMMAND "toolset=msvc-${BOOST_TOOLSET_MSVC_VERSION}")
  else()
    list(APPEND BOOST_BUILD_COMMAND "cxxflags=-fPIC")
  endif()

  add_thirdparty_lib(boost_system STATIC_LIB "${BOOST_STATIC_SYSTEM_LIBRARY}")

  add_thirdparty_lib(boost_filesystem STATIC_LIB "${BOOST_STATIC_FILESYSTEM_LIBRARY}")

  add_thirdparty_lib(boost_regex STATIC_LIB "${BOOST_STATIC_REGEX_LIBRARY}")

  externalproject_add(boost_ep
                      URL ${BOOST_SOURCE_URL}
                      BUILD_BYPRODUCTS ${BOOST_BUILD_PRODUCTS}
                      BUILD_IN_SOURCE 1
                      CONFIGURE_COMMAND ${BOOST_CONFIGURE_COMMAND}
                      BUILD_COMMAND ${BOOST_BUILD_COMMAND}
                      INSTALL_COMMAND "" ${EP_LOG_OPTIONS})
  set(Boost_INCLUDE_DIR "${BOOST_PREFIX}")
  set(Boost_INCLUDE_DIRS "${BOOST_INCLUDE_DIR}")
  add_dependencies(toolchain boost_ep)
  set(BOOST_VENDORED TRUE)
endmacro()


set(PEGASUS_BOOST_REQUIRED_VERSION "1.64")


set(Boost_USE_MULTITHREADED ON)
if(MSVC AND PEGASUS_USE_STATIC_CRT)
  set(Boost_USE_STATIC_RUNTIME ON)
endif()
set(Boost_ADDITIONAL_VERSIONS
    "1.73.0"
    "1.73"
    "1.72.0"
    "1.72"
    "1.71.0"
    "1.71"
    "1.70.0"
    "1.70"
    "1.69.0"
    "1.69"
    "1.68.0"
    "1.68"
    "1.67.0"
    "1.67"
    "1.66.0"
    "1.66"
    "1.65.0"
    "1.65"
    "1.64.0"
    "1.64"
    "1.63.0"
    "1.63"
    "1.62.0"
    "1.61"
    "1.61.0"
    "1.62"
    "1.60.0"
    "1.60")

set(PEGASUS_BOOST_REQUIRED TRUE)

if(PEGASUS_BOOST_REQUIRED)
  if(BOOST_SOURCE STREQUAL "AUTO")
    find_package(BoostAlt ${PEGASUS_BOOST_REQUIRED_VERSION})
    if(NOT BoostAlt_FOUND)
      build_boost()
    endif()
  elseif(BOOST_SOURCE STREQUAL "BUNDLED")
    build_boost()
  elseif(BOOST_SOURCE STREQUAL "SYSTEM")
    find_package(BoostAlt ${PEGASUS_BOOST_REQUIRED_VERSION} REQUIRED)
  endif()

  if(TARGET Boost::system)
    set(BOOST_SYSTEM_LIBRARY Boost::system)
    set(BOOST_FILESYSTEM_LIBRARY Boost::filesystem)
    set(BOOST_REGEX_LIBRARY Boost::regex)
  elseif(BoostAlt_FOUND)
    set(BOOST_SYSTEM_LIBRARY ${Boost_SYSTEM_LIBRARY})
    set(BOOST_FILESYSTEM_LIBRARY ${Boost_FILESYSTEM_LIBRARY})
    set(BOOST_REGEX_LIBRARY ${Boost_REGEX_LIBRARY})
  else()
    set(BOOST_SYSTEM_LIBRARY boost_system_static)
    set(BOOST_FILESYSTEM_LIBRARY boost_filesystem_static)
    set(BOOST_REGEX_LIBRARY boost_regex_static)
  endif()
  set(PEGASUS_BOOST_LIBS ${BOOST_SYSTEM_LIBRARY} ${BOOST_FILESYSTEM_LIBRARY})

  message(STATUS "Boost include dir: ${Boost_INCLUDE_DIR}")
  message(STATUS "Boost libraries: ${PEGASUS_BOOST_LIBS}")

  include_directories(SYSTEM ${Boost_INCLUDE_DIR})
endif()

# ----------------------------------------------------------------------
# uriparser library

macro(build_uriparser)
  message(STATUS "Building uriparser from source")
  set(URIPARSER_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/uriparser_ep-install")
  set(
    URIPARSER_STATIC_LIB
    "${URIPARSER_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}uriparser${CMAKE_STATIC_LIBRARY_SUFFIX}"
    )
  set(URIPARSER_INCLUDE_DIRS "${URIPARSER_PREFIX}/include")

  set(URIPARSER_CMAKE_ARGS
      ${EP_COMMON_CMAKE_ARGS}
      "-DURIPARSER_BUILD_DOCS=off"
      "-DURIPARSER_BUILD_TESTS=off"
      "-DURIPARSER_BUILD_TOOLS=off"
      "-DURIPARSER_BUILD_WCHAR_T=off"
      "-DBUILD_SHARED_LIBS=off"
      "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
      "-DCMAKE_INSTALL_LIBDIR=lib"
      "-DCMAKE_POSITION_INDEPENDENT_CODE=on"
      "-DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>")

  if(MSVC AND PEGASUS_USE_STATIC_CRT)
    if("${CMAKE_BUILD_TYPE}" STREQUAL "DEBUG")
      list(APPEND URIPARSER_CMAKE_ARGS "-DURIPARSER_MSVC_RUNTIME=/MTd")
    else()
      list(APPEND URIPARSER_CMAKE_ARGS "-DURIPARSER_MSVC_RUNTIME=/MT")
    endif()
  endif()

  externalproject_add(uriparser_ep
                      URL ${URIPARSER_SOURCE_URL}
                      CMAKE_ARGS ${URIPARSER_CMAKE_ARGS}
                      BUILD_BYPRODUCTS ${URIPARSER_STATIC_LIB}
                      INSTALL_DIR ${URIPARSER_PREFIX}
                      ${EP_LOG_OPTIONS})

  add_library(uriparser::uriparser STATIC IMPORTED)
  # Work around https://gitlab.kitware.com/cmake/cmake/issues/15052
  file(MAKE_DIRECTORY ${URIPARSER_INCLUDE_DIRS})
  set_target_properties(uriparser::uriparser
                        PROPERTIES IMPORTED_LOCATION ${URIPARSER_STATIC_LIB}
                                   INTERFACE_INCLUDE_DIRECTORIES ${URIPARSER_INCLUDE_DIRS}
                                   # URI_STATIC_BUILD required on Windows
                                   INTERFACE_COMPILE_DEFINITIONS
                                   "URI_STATIC_BUILD;URI_NO_UNICODE")

  add_dependencies(toolchain uriparser_ep)
  add_dependencies(uriparser::uriparser uriparser_ep)
endmacro()

if(PEGASUS_WITH_URIPARSER)
  set(PEGASUS_URIPARSER_REQUIRED_VERSION "0.9.0")
  if(uriparser_SOURCE STREQUAL "AUTO")
    # Debian does not ship cmake configs for uriparser
    find_package(uriparser ${PEGASUS_URIPARSER_REQUIRED_VERSION} QUIET)
    if(NOT uriparser_FOUND)
      find_package(uriparserAlt ${PEGASUS_URIPARSER_REQUIRED_VERSION})
    endif()
    if(NOT uriparser_FOUND AND NOT uriparserAlt_FOUND)
      build_uriparser()
    endif()
  elseif(uriparser_SOURCE STREQUAL "BUNDLED")
    build_uriparser()
  elseif(uriparser_SOURCE STREQUAL "SYSTEM")
    # Debian does not ship cmake configs for uriparser
    find_package(uriparser ${PEGASUS_URIPARSER_REQUIRED_VERSION} QUIET)
    if(NOT uriparser_FOUND)
      find_package(uriparserAlt ${PEGASUS_URIPARSER_REQUIRED_VERSION} REQUIRED)
    endif()
  endif()

  get_target_property(URIPARSER_INCLUDE_DIRS uriparser::uriparser
                      INTERFACE_INCLUDE_DIRECTORIES)
  include_directories(SYSTEM ${URIPARSER_INCLUDE_DIRS})
endif()

# ----------------------------------------------------------------------
# Snappy

macro(build_snappy)
  message(STATUS "Building snappy from source")
  set(SNAPPY_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/snappy_ep/src/snappy_ep-install")
  set(SNAPPY_STATIC_LIB_NAME snappy)
  set(
    SNAPPY_STATIC_LIB
    "${SNAPPY_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}${SNAPPY_STATIC_LIB_NAME}${CMAKE_STATIC_LIBRARY_SUFFIX}"
    )

  set(SNAPPY_CMAKE_ARGS ${EP_COMMON_CMAKE_ARGS} -DCMAKE_INSTALL_LIBDIR=lib
                        -DSNAPPY_BUILD_TESTS=OFF
                        "-DCMAKE_INSTALL_PREFIX=${SNAPPY_PREFIX}")

  externalproject_add(snappy_ep
                      ${EP_LOG_OPTIONS}
                      BUILD_IN_SOURCE 1
                      INSTALL_DIR ${SNAPPY_PREFIX}
                      URL ${SNAPPY_SOURCE_URL}
                      CMAKE_ARGS ${SNAPPY_CMAKE_ARGS}
                      BUILD_BYPRODUCTS "${SNAPPY_STATIC_LIB}")

  file(MAKE_DIRECTORY "${SNAPPY_PREFIX}/include")

  add_library(Snappy::snappy STATIC IMPORTED)
  set_target_properties(Snappy::snappy
                        PROPERTIES IMPORTED_LOCATION "${SNAPPY_STATIC_LIB}"
                                   INTERFACE_INCLUDE_DIRECTORIES
                                   "${SNAPPY_PREFIX}/include")
  add_dependencies(toolchain snappy_ep)
  add_dependencies(Snappy::snappy snappy_ep)
endmacro()

if(PEGASUS_WITH_SNAPPY)
  if(Snappy_SOURCE STREQUAL "AUTO")
    # Normally *Config.cmake files reside in /usr/lib/cmake but Snappy
    # errornously places them in ${CMAKE_ROOT}/Modules/
    # This is fixed in 1.1.7 but fedora (30) still installs into the wrong
    # location.
    # https://bugzilla.redhat.com/show_bug.cgi?id=1679727
    # https://src.fedoraproject.org/rpms/snappy/pull-request/1
    find_package(Snappy QUIET HINTS "${CMAKE_ROOT}/Modules/")
    if(NOT Snappy_FOUND)
      find_package(SnappyAlt)
    endif()
    if(NOT Snappy_FOUND AND NOT SnappyAlt_FOUND)
      build_snappy()
    endif()
  elseif(Snappy_SOURCE STREQUAL "BUNDLED")
    build_snappy()
  elseif(Snappy_SOURCE STREQUAL "SYSTEM")
    # SnappyConfig.cmake is not installed on Ubuntu/Debian
    # TODO: Make a bug report upstream
    find_package(Snappy HINTS "${CMAKE_ROOT}/Modules/")
    if(NOT Snappy_FOUND)
      find_package(SnappyAlt REQUIRED)
    endif()
  endif()

  # TODO: Don't use global includes but rather target_include_directories
  get_target_property(SNAPPY_INCLUDE_DIRS Snappy::snappy INTERFACE_INCLUDE_DIRECTORIES)
  include_directories(SYSTEM ${SNAPPY_INCLUDE_DIRS})
endif()


if(PARQUET_REQUIRE_ENCRYPTION AND NOT PEGASUS_PARQUET)
  set(PARQUET_REQUIRE_ENCRYPTION OFF)
endif()
set(PEGASUS_OPENSSL_REQUIRED_VERSION "1.0.2")
if(BREW_BIN AND NOT OPENSSL_ROOT_DIR)
  execute_process(COMMAND ${BREW_BIN} --prefix "openssl"
                  OUTPUT_VARIABLE OPENSSL_BREW_PREFIX
                  OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(OPENSSL_BREW_PREFIX)
    set(OPENSSL_ROOT_DIR ${OPENSSL_BREW_PREFIX})
  endif()
endif()

set(PEGASUS_USE_OPENSSL OFF)
if(PARQUET_REQUIRE_ENCRYPTION OR ARROW_FLIGHT)
  # This must work
  find_package(OpenSSL ${PEGASUS_OPENSSL_REQUIRED_VERSION} REQUIRED)
  set(PEGASUS_USE_OPENSSL ON)
endif()

if(PEGASUS_USE_OPENSSL)
  message(STATUS "Found OpenSSL Crypto Library: ${OPENSSL_CRYPTO_LIBRARY}")
  message(STATUS "Building with OpenSSL (Version: ${OPENSSL_VERSION}) support")

  # OpenSSL::SSL and OpenSSL::Crypto were not added to
  # FindOpenSSL.cmake until version 3.4.0.
  # https://gitlab.kitware.com/cmake/cmake/blob/75e3a8e811b290cb9921887f2b086377af90880f/Modules/FindOpenSSL.cmake
  if(NOT TARGET OpenSSL::SSL)
    add_library(OpenSSL::SSL UNKNOWN IMPORTED)
    set_target_properties(OpenSSL::SSL
                          PROPERTIES IMPORTED_LOCATION "${OPENSSL_SSL_LIBRARY}"
                                     INTERFACE_INCLUDE_DIRECTORIES
                                     "${OPENSSL_INCLUDE_DIR}")

    add_library(OpenSSL::Crypto UNKNOWN IMPORTED)
    set_target_properties(OpenSSL::Crypto
                          PROPERTIES IMPORTED_LOCATION "${OPENSSL_CRYPTO_LIBRARY}"
                                     INTERFACE_INCLUDE_DIRECTORIES
                                     "${OPENSSL_INCLUDE_DIR}")
  endif()

  include_directories(SYSTEM ${OPENSSL_INCLUDE_DIR})
else()
  message(
    STATUS
      "Building without OpenSSL support. Minimum OpenSSL version ${PEGASUS_OPENSSL_REQUIRED_VERSION} required."
    )
endif()

# ----------------------------------------------------------------------
# GLOG

macro(build_glog)
  message(STATUS "Building glog from source")
  set(GLOG_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/glog_ep-prefix/src/glog_ep")
  set(GLOG_INCLUDE_DIR "${GLOG_BUILD_DIR}/include")
  set(
    GLOG_STATIC_LIB
    "${GLOG_BUILD_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}glog${CMAKE_STATIC_LIBRARY_SUFFIX}"
    )
  set(GLOG_CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
  set(GLOG_CMAKE_C_FLAGS "${EP_C_FLAGS} -fPIC")
  if(Threads::Threads)
    set(GLOG_CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -pthread")
    set(GLOG_CMAKE_C_FLAGS "${EP_C_FLAGS} -fPIC -pthread")
  endif()

  if(APPLE)
    # If we don't set this flag, the binary built with 10.13 cannot be used in 10.12.
    set(GLOG_CMAKE_CXX_FLAGS "${GLOG_CMAKE_CXX_FLAGS} -mmacosx-version-min=10.9")
  endif()

  set(GLOG_CMAKE_ARGS
      ${EP_COMMON_CMAKE_ARGS}
      "-DCMAKE_INSTALL_PREFIX=${GLOG_BUILD_DIR}"
      -DBUILD_SHARED_LIBS=OFF
      -DBUILD_TESTING=OFF
      -DWITH_GFLAGS=OFF
      -DCMAKE_CXX_FLAGS_${UPPERCASE_BUILD_TYPE}=${GLOG_CMAKE_CXX_FLAGS}
      -DCMAKE_C_FLAGS_${UPPERCASE_BUILD_TYPE}=${GLOG_CMAKE_C_FLAGS}
      -DCMAKE_CXX_FLAGS=${GLOG_CMAKE_CXX_FLAGS})
  externalproject_add(glog_ep
                      URL ${GLOG_SOURCE_URL}
                      BUILD_IN_SOURCE 1
                      BUILD_BYPRODUCTS "${GLOG_STATIC_LIB}"
                      CMAKE_ARGS ${GLOG_CMAKE_ARGS} ${EP_LOG_OPTIONS})

  add_dependencies(toolchain glog_ep)
  file(MAKE_DIRECTORY "${GLOG_INCLUDE_DIR}")

  add_library(GLOG::glog STATIC IMPORTED)
  set_target_properties(GLOG::glog
                        PROPERTIES IMPORTED_LOCATION "${GLOG_STATIC_LIB}"
                                   INTERFACE_INCLUDE_DIRECTORIES "${GLOG_INCLUDE_DIR}")
  add_dependencies(GLOG::glog glog_ep)
endmacro()

if(PEGASUS_USE_GLOG)
  resolve_dependency(GLOG)
  # TODO: Don't use global includes but rather target_include_directories
  get_target_property(GLOG_INCLUDE_DIR GLOG::glog INTERFACE_INCLUDE_DIRECTORIES)
  include_directories(SYSTEM ${GLOG_INCLUDE_DIR})
endif()

# ----------------------------------------------------------------------
# gflags

set(PEGASUS_NEED_GFLAGS 1)


macro(build_gflags)
  message(STATUS "Building gflags from source")

  set(GFLAGS_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/gflags_ep-prefix/src/gflags_ep")
  set(GFLAGS_INCLUDE_DIR "${GFLAGS_PREFIX}/include")
  if(MSVC)
    set(GFLAGS_STATIC_LIB "${GFLAGS_PREFIX}/lib/gflags_static.lib")
  else()
    set(GFLAGS_STATIC_LIB "${GFLAGS_PREFIX}/lib/libgflags.a")
  endif()
  set(GFLAGS_CMAKE_ARGS
      ${EP_COMMON_CMAKE_ARGS}
      "-DCMAKE_INSTALL_PREFIX=${GFLAGS_PREFIX}"
      -DBUILD_SHARED_LIBS=OFF
      -DBUILD_STATIC_LIBS=ON
      -DBUILD_PACKAGING=OFF
      -DBUILD_TESTING=OFF
      -DBUILD_CONFIG_TESTS=OFF
      -DINSTALL_HEADERS=ON)

  file(MAKE_DIRECTORY "${GFLAGS_INCLUDE_DIR}")
  externalproject_add(gflags_ep
                      URL ${GFLAGS_SOURCE_URL} ${EP_LOG_OPTIONS}
                      BUILD_IN_SOURCE 1
                      BUILD_BYPRODUCTS "${GFLAGS_STATIC_LIB}"
                      CMAKE_ARGS ${GFLAGS_CMAKE_ARGS})

  add_dependencies(toolchain gflags_ep)

  add_thirdparty_lib(gflags STATIC_LIB ${GFLAGS_STATIC_LIB})
  set(GFLAGS_LIBRARY gflags_static)
  set_target_properties(${GFLAGS_LIBRARY}
                        PROPERTIES INTERFACE_COMPILE_DEFINITIONS "GFLAGS_IS_A_DLL=0"
                                   INTERFACE_INCLUDE_DIRECTORIES "${GFLAGS_INCLUDE_DIR}")
  if(MSVC)
    set_target_properties(${GFLAGS_LIBRARY}
                          PROPERTIES INTERFACE_LINK_LIBRARIES "shlwapi.lib")
  endif()
  set(GFLAGS_LIBRARIES ${GFLAGS_LIBRARY})

  set(GFLAGS_VENDORED TRUE)
endmacro()

if(PEGASUS_NEED_GFLAGS)
  if(gflags_SOURCE STREQUAL "AUTO")
    find_package(gflags QUIET)
    if(NOT gflags_FOUND)
      find_package(gflagsAlt)
    endif()
    if(NOT gflags_FOUND AND NOT gflagsAlt_FOUND)
      build_gflags()
    endif()
  elseif(gflags_SOURCE STREQUAL "BUNDLED")
    build_gflags()
  elseif(gflags_SOURCE STREQUAL "SYSTEM")
    # gflagsConfig.cmake is not installed on Ubuntu/Debian
    # TODO: Make a bug report upstream
    find_package(gflags)
    if(NOT gflags_FOUND)
      find_package(gflagsAlt REQUIRED)
    endif()
  endif()
  # TODO: Don't use global includes but rather target_include_directories
  include_directories(SYSTEM ${GFLAGS_INCLUDE_DIR})

  if(NOT TARGET ${GFLAGS_LIBRARIES})
    if(TARGET gflags-shared)
      set(GFLAGS_LIBRARIES gflags-shared)
    elseif(TARGET gflags_shared)
      set(GFLAGS_LIBRARIES gflags_shared)
    endif()
  endif()
endif()

# ----------------------------------------------------------------------
# Thrift

macro(build_thrift)
  message("Building Apache Thrift from source")
  set(THRIFT_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/thrift_ep/src/thrift_ep-install")
  set(THRIFT_INCLUDE_DIR "${THRIFT_PREFIX}/include")
  set(THRIFT_COMPILER "${THRIFT_PREFIX}/bin/thrift")
  set(THRIFT_CMAKE_ARGS
      ${EP_COMMON_CMAKE_ARGS}
      "-DCMAKE_INSTALL_PREFIX=${THRIFT_PREFIX}"
      "-DCMAKE_INSTALL_RPATH=${THRIFT_PREFIX}/lib"
      -DBUILD_SHARED_LIBS=OFF
      -DBUILD_TESTING=OFF
      -DBUILD_EXAMPLES=OFF
      -DBUILD_TUTORIALS=OFF
      -DWITH_QT4=OFF
      -DWITH_C_GLIB=OFF
      -DWITH_JAVA=OFF
      -DWITH_PYTHON=OFF
      -DWITH_HASKELL=OFF
      -DWITH_CPP=ON
      -DWITH_STATIC_LIB=ON
      -DWITH_LIBEVENT=OFF
      # Work around https://gitlab.kitware.com/cmake/cmake/issues/18865
      -DBoost_NO_BOOST_CMAKE=ON)

  # Thrift also uses boost. Forward important boost settings if there were ones passed.
  if(DEFINED BOOST_ROOT)
    set(THRIFT_CMAKE_ARGS ${THRIFT_CMAKE_ARGS} "-DBOOST_ROOT=${BOOST_ROOT}")
  endif()
  if(DEFINED Boost_NAMESPACE)
    set(THRIFT_CMAKE_ARGS ${THRIFT_CMAKE_ARGS} "-DBoost_NAMESPACE=${Boost_NAMESPACE}")
  endif()

  set(THRIFT_STATIC_LIB_NAME "${CMAKE_STATIC_LIBRARY_PREFIX}thrift")
  if(MSVC)
    if(PEGASUS_USE_STATIC_CRT)
      set(THRIFT_STATIC_LIB_NAME "${THRIFT_STATIC_LIB_NAME}mt")
      set(THRIFT_CMAKE_ARGS ${THRIFT_CMAKE_ARGS} "-DWITH_MT=ON")
    else()
      set(THRIFT_STATIC_LIB_NAME "${THRIFT_STATIC_LIB_NAME}md")
      set(THRIFT_CMAKE_ARGS ${THRIFT_CMAKE_ARGS} "-DWITH_MT=OFF")
    endif()
  endif()
  if(${UPPERCASE_BUILD_TYPE} STREQUAL "DEBUG")
    set(THRIFT_STATIC_LIB_NAME "${THRIFT_STATIC_LIB_NAME}d")
  endif()
  set(THRIFT_STATIC_LIB
      "${THRIFT_PREFIX}/lib/${THRIFT_STATIC_LIB_NAME}${CMAKE_STATIC_LIBRARY_SUFFIX}")

  if(ZLIB_SHARED_LIB)
    set(THRIFT_CMAKE_ARGS "-DZLIB_LIBRARY=${ZLIB_SHARED_LIB}" ${THRIFT_CMAKE_ARGS})
  else()
    set(THRIFT_CMAKE_ARGS "-DZLIB_LIBRARY=${ZLIB_STATIC_LIB}" ${THRIFT_CMAKE_ARGS})
  endif()
  set(THRIFT_DEPENDENCIES ${THRIFT_DEPENDENCIES} ${ZLIB_LIBRARY})

  if(BOOST_VENDORED)
    set(THRIFT_DEPENDENCIES ${THRIFT_DEPENDENCIES} boost_ep)
  endif()

  if("${THRIFT_SOURCE_URL}" STREQUAL "FROM-APACHE-MIRROR")
    get_apache_mirror()
    set(THRIFT_SOURCE_URL
        "${APACHE_MIRROR}/thrift/${THRIFT_VERSION}/thrift-${THRIFT_VERSION}.tar.gz")
  endif()

  message("Downloading Apache Thrift from ${THRIFT_SOURCE_URL}")

  externalproject_add(thrift_ep
                      URL ${THRIFT_SOURCE_URL}
                      URL_HASH "MD5=${THRIFT_MD5_CHECKSUM}"
                      BUILD_BYPRODUCTS "${THRIFT_STATIC_LIB}" "${THRIFT_COMPILER}"
                      CMAKE_ARGS ${THRIFT_CMAKE_ARGS}
                      DEPENDS ${THRIFT_DEPENDENCIES} ${EP_LOG_OPTIONS})

  add_library(Thrift::thrift STATIC IMPORTED)
  # The include directory must exist before it is referenced by a target.
  file(MAKE_DIRECTORY "${THRIFT_INCLUDE_DIR}")
  set_target_properties(Thrift::thrift
                        PROPERTIES IMPORTED_LOCATION "${THRIFT_STATIC_LIB}"
                                   INTERFACE_INCLUDE_DIRECTORIES "${THRIFT_INCLUDE_DIR}")
  add_dependencies(toolchain thrift_ep)
  add_dependencies(Thrift::thrift thrift_ep)
endmacro()

if(PEGASUS_WITH_THRIFT)
  resolve_dependency(Thrift)
  # TODO: Don't use global includes but rather target_include_directories
  include_directories(SYSTEM ${THRIFT_INCLUDE_DIR})

  if(THRIFT_VERSION VERSION_LESS "0.11.0")
    add_definitions(-DPARQUET_THRIFT_USE_BOOST)
  endif()
endif()

# ----------------------------------------------------------------------
# Protocol Buffers (required for ORC and Flight and Gandiva libraries)

macro(build_protobuf)
  message("Building Protocol Buffers from source")
  set(PROTOBUF_PREFIX "${THIRDPARTY_DIR}/protobuf_ep-install")
  set(PROTOBUF_INCLUDE_DIR "${PROTOBUF_PREFIX}/include")
  set(
    PROTOBUF_STATIC_LIB
    "${PROTOBUF_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}protobuf${CMAKE_STATIC_LIBRARY_SUFFIX}"
    )
  set(
    PROTOC_STATIC_LIB
    "${PROTOBUF_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}protoc${CMAKE_STATIC_LIBRARY_SUFFIX}"
    )
  set(PROTOBUF_COMPILER "${PROTOBUF_PREFIX}/bin/protoc")
  set(PROTOBUF_CONFIGURE_ARGS
      "AR=${CMAKE_AR}"
      "RANLIB=${CMAKE_RANLIB}"
      "CC=${CMAKE_C_COMPILER}"
      "CXX=${CMAKE_CXX_COMPILER}"
      "--disable-shared"
      "--prefix=${PROTOBUF_PREFIX}"
      "CFLAGS=${EP_C_FLAGS}"
      "CXXFLAGS=${EP_CXX_FLAGS}")

  externalproject_add(protobuf_ep
                      CONFIGURE_COMMAND "./configure" ${PROTOBUF_CONFIGURE_ARGS}
                      BUILD_COMMAND ${MAKE} ${MAKE_BUILD_ARGS}
                      BUILD_IN_SOURCE 1
                      URL ${PROTOBUF_SOURCE_URL}
                      BUILD_BYPRODUCTS "${PROTOBUF_STATIC_LIB}" "${PROTOBUF_COMPILER}"
                                       ${EP_LOG_OPTIONS})

  file(MAKE_DIRECTORY "${PROTOBUF_INCLUDE_DIR}")

  add_library(protobuf::libprotobuf STATIC IMPORTED)
  set_target_properties(
    protobuf::libprotobuf
    PROPERTIES IMPORTED_LOCATION "${PROTOBUF_STATIC_LIB}" INTERFACE_INCLUDE_DIRECTORIES
               "${PROTOBUF_INCLUDE_DIR}")
  add_library(protobuf::libprotoc STATIC IMPORTED)
  set_target_properties(
    protobuf::libprotoc
    PROPERTIES IMPORTED_LOCATION "${PROTOC_STATIC_LIB}" INTERFACE_INCLUDE_DIRECTORIES
               "${PROTOBUF_INCLUDE_DIR}")
  add_executable(protobuf::protoc IMPORTED)
  set_target_properties(protobuf::protoc
                        PROPERTIES IMPORTED_LOCATION "${PROTOBUF_COMPILER}")

  add_dependencies(toolchain protobuf_ep)
  add_dependencies(protobuf::libprotobuf protobuf_ep)
endmacro()

if(PEGASUS_WITH_PROTOBUF)
  if(PEGASUS_WITH_GRPC)
    # gRPC 1.21.0 or later require Protobuf 3.7.0 or later.
    set(PEGASUS_PROTOBUF_REQUIRED_VERSION "3.7.0")
  else()
    set(PEGASUS_PROTOBUF_REQUIRED_VERSION "2.6.1")
  endif()
  resolve_dependency_with_version(Protobuf ${PEGASUS_PROTOBUF_REQUIRED_VERSION})

  if(PEGASUS_PROTOBUF_USE_SHARED AND MSVC)
    add_definitions(-DPROTOBUF_USE_DLLS)
  endif()

  # TODO: Don't use global includes but rather target_include_directories
  include_directories(SYSTEM ${PROTOBUF_INCLUDE_DIR})

  # Old CMake versions don't define the targets
  if(NOT TARGET protobuf::libprotobuf)
    add_library(protobuf::libprotobuf UNKNOWN IMPORTED)
    set_target_properties(protobuf::libprotobuf
                          PROPERTIES IMPORTED_LOCATION "${PROTOBUF_LIBRARY}"
                                     INTERFACE_INCLUDE_DIRECTORIES
                                     "${PROTOBUF_INCLUDE_DIR}")
  endif()
  if(NOT TARGET protobuf::libprotoc)
    if(PROTOBUF_PROTOC_LIBRARY AND NOT Protobuf_PROTOC_LIBRARY)
      # Old CMake versions have a different casing.
      set(Protobuf_PROTOC_LIBRARY ${PROTOBUF_PROTOC_LIBRARY})
    endif()
    if(NOT Protobuf_PROTOC_LIBRARY)
      message(FATAL_ERROR "libprotoc was set to ${Protobuf_PROTOC_LIBRARY}")
    endif()
    add_library(protobuf::libprotoc UNKNOWN IMPORTED)
    set_target_properties(protobuf::libprotoc
                          PROPERTIES IMPORTED_LOCATION "${Protobuf_PROTOC_LIBRARY}"
                                     INTERFACE_INCLUDE_DIRECTORIES
                                     "${PROTOBUF_INCLUDE_DIR}")
  endif()
  if(NOT TARGET protobuf::protoc)
    add_executable(protobuf::protoc IMPORTED)
    set_target_properties(protobuf::protoc
                          PROPERTIES IMPORTED_LOCATION "${PROTOBUF_PROTOC_EXECUTABLE}")
  endif()

  # Log protobuf paths as we often see issues with mixed sources for
  # the libraries and protoc.
  get_target_property(PROTOBUF_PROTOC_EXECUTABLE protobuf::protoc IMPORTED_LOCATION)
  message(STATUS "Found protoc: ${PROTOBUF_PROTOC_EXECUTABLE}")
  # Protobuf_PROTOC_LIBRARY is set by all versions of FindProtobuf.cmake
  message(STATUS "Found libprotoc: ${Protobuf_PROTOC_LIBRARY}")
  get_target_property(PROTOBUF_LIBRARY protobuf::libprotobuf IMPORTED_LOCATION)
  message(STATUS "Found libprotobuf: ${PROTOBUF_LIBRARY}")
  message(STATUS "Found protobuf headers: ${PROTOBUF_INCLUDE_DIR}")
endif()

# ----------------------------------------------------------------------
# jemalloc - Unix-only high-performance allocator

if(WIN32)
  # jemalloc is not supported on Windows
  set(PEGASUS_JEMALLOC off)
endif()

if(PEGASUS_JEMALLOC)
  message(STATUS "Building (vendored) jemalloc from source")
  # We only use a vendored jemalloc as we want to control its version.
  # Also our build of jemalloc is specially prefixed so that it will not
  # conflict with the default allocator as well as other jemalloc
  # installations.
  # find_package(jemalloc)

  set(PEGASUS_JEMALLOC_USE_SHARED OFF)
  set(JEMALLOC_PREFIX
      "${CMAKE_CURRENT_BINARY_DIR}/jemalloc_ep-prefix/src/jemalloc_ep/dist/")
  set(JEMALLOC_STATIC_LIB
      "${JEMALLOC_PREFIX}/lib/libjemalloc_pic${CMAKE_STATIC_LIBRARY_SUFFIX}")
  set(JEMALLOC_CONFIGURE_COMMAND ./configure "AR=${CMAKE_AR}" "CC=${CMAKE_C_COMPILER}")
  if(CMAKE_OSX_SYSROOT)
    list(APPEND JEMALLOC_CONFIGURE_COMMAND "SDKROOT=${CMAKE_OSX_SYSROOT}")
  endif()
  list(APPEND JEMALLOC_CONFIGURE_COMMAND
              "--prefix=${JEMALLOC_PREFIX}"
              "--with-jemalloc-prefix=je_pegasus_"
              "--with-private-namespace=je_pegasus_private_"
              "--without-export"
              # Don't override operator new()
              "--disable-cxx" "--disable-libdl"
              # See https://github.com/jemalloc/jemalloc/issues/1237
              "--disable-initial-exec-tls" ${EP_LOG_OPTIONS})
  set(JEMALLOC_BUILD_COMMAND ${MAKE} ${MAKE_BUILD_ARGS})
  if(CMAKE_OSX_SYSROOT)
    list(APPEND JEMALLOC_BUILD_COMMAND "SDKROOT=${CMAKE_OSX_SYSROOT}")
  endif()
  externalproject_add(
    jemalloc_ep
    URL ${JEMALLOC_SOURCE_URL}
    PATCH_COMMAND
      touch doc/jemalloc.3 doc/jemalloc.html
      # The prefix "je_pegasus_" must be kept in sync with the value in memory_pool.cc
    CONFIGURE_COMMAND ${JEMALLOC_CONFIGURE_COMMAND}
    BUILD_IN_SOURCE 1
    BUILD_COMMAND ${JEMALLOC_BUILD_COMMAND}
    BUILD_BYPRODUCTS "${JEMALLOC_STATIC_LIB}"
    INSTALL_COMMAND ${MAKE} install)

  # Don't use the include directory directly so that we can point to a path
  # that is unique to our codebase.
  include_directories(SYSTEM "${CMAKE_CURRENT_BINARY_DIR}/jemalloc_ep-prefix/src/")
  # The include directory must exist before it is referenced by a target.
  file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/jemalloc_ep-prefix/src/")
  add_library(jemalloc::jemalloc STATIC IMPORTED)
  set_target_properties(jemalloc::jemalloc
                        PROPERTIES INTERFACE_LINK_LIBRARIES
                                   Threads::Threads
                                   IMPORTED_LOCATION
                                   "${JEMALLOC_STATIC_LIB}"
                                   INTERFACE_INCLUDE_DIRECTORIES
                                   "${CMAKE_CURRENT_BINARY_DIR}/jemalloc_ep-prefix/src")
  add_dependencies(jemalloc::jemalloc jemalloc_ep)
endif()

# ----------------------------------------------------------------------
# mimalloc - Cross-platform high-performance allocator, from Microsoft

if(PEGASUS_MIMALLOC)
  message(STATUS "Building (vendored) mimalloc from source")
  # We only use a vendored mimalloc as we want to control its build options.

  # XXX Careful: mimalloc library naming varies depend on build type capitalization:
  # https://github.com/microsoft/mimalloc/issues/144
  set(MIMALLOC_LIB_BASE_NAME "mimalloc")
  if(WIN32)
    set(MIMALLOC_LIB_BASE_NAME "${MIMALLOC_LIB_BASE_NAME}-static")
  endif()
  set(MIMALLOC_LIB_BASE_NAME "${MIMALLOC_LIB_BASE_NAME}-${LOWERCASE_BUILD_TYPE}")

  set(MIMALLOC_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/mimalloc_ep/src/mimalloc_ep")
  set(MIMALLOC_INCLUDE_DIR "${MIMALLOC_PREFIX}/lib/mimalloc-1.0/include")
  set(
    MIMALLOC_STATIC_LIB
    "${MIMALLOC_PREFIX}/lib/mimalloc-1.0/${CMAKE_STATIC_LIBRARY_PREFIX}${MIMALLOC_LIB_BASE_NAME}${CMAKE_STATIC_LIBRARY_SUFFIX}"
    )

  set(MIMALLOC_CMAKE_ARGS
      ${EP_COMMON_CMAKE_ARGS}
      "-DCMAKE_INSTALL_PREFIX=${MIMALLOC_PREFIX}"
      -DMI_OVERRIDE=OFF
      -DMI_LOCAL_DYNAMIC_TLS=ON
      -DMI_BUILD_TESTS=OFF)

  externalproject_add(mimalloc_ep
                      URL ${MIMALLOC_SOURCE_URL}
                      CMAKE_ARGS ${MIMALLOC_CMAKE_ARGS}
                      BUILD_BYPRODUCTS "${MIMALLOC_STATIC_LIB}")

  include_directories(SYSTEM ${MIMALLOC_INCLUDE_DIR})
  file(MAKE_DIRECTORY ${MIMALLOC_INCLUDE_DIR})

  add_library(mimalloc::mimalloc STATIC IMPORTED)
  set_target_properties(mimalloc::mimalloc
                        PROPERTIES INTERFACE_LINK_LIBRARIES
                                   Threads::Threads
                                   IMPORTED_LOCATION
                                   "${MIMALLOC_STATIC_LIB}"
                                   INTERFACE_INCLUDE_DIRECTORIES
                                   "${MIMALLOC_INCLUDE_DIR}")
  add_dependencies(mimalloc::mimalloc mimalloc_ep)
  add_dependencies(toolchain mimalloc_ep)
endif()

# ----------------------------------------------------------------------
# Google gtest

macro(build_gtest)
  message(STATUS "Building gtest from source")
  set(GTEST_VENDORED TRUE)
  set(GTEST_CMAKE_CXX_FLAGS ${EP_CXX_FLAGS})

  if(CMAKE_BUILD_TYPE MATCHES DEBUG)
    set(CMAKE_GTEST_DEBUG_EXTENSION "d")
  else()
    set(CMAKE_GTEST_DEBUG_EXTENSION "")
  endif()

  if(APPLE)
    set(GTEST_CMAKE_CXX_FLAGS ${GTEST_CMAKE_CXX_FLAGS} -DGTEST_USE_OWN_TR1_TUPLE=1
                              -Wno-unused-value -Wno-ignored-attributes)
  endif()

  if(MSVC)
    set(GTEST_CMAKE_CXX_FLAGS "${GTEST_CMAKE_CXX_FLAGS} -DGTEST_CREATE_SHARED_LIBRARY=1")
  endif()

  set(GTEST_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/googletest_ep-prefix/src/googletest_ep")
  set(GTEST_INCLUDE_DIR "${GTEST_PREFIX}/include")

  set(_GTEST_RUNTIME_DIR ${BUILD_OUTPUT_ROOT_DIRECTORY})

  if(MSVC)
    set(_GTEST_IMPORTED_TYPE IMPORTED_IMPLIB)
    set(_GTEST_LIBRARY_SUFFIX
        "${CMAKE_GTEST_DEBUG_EXTENSION}${CMAKE_IMPORT_LIBRARY_SUFFIX}")
    # Use the import libraries from the EP
    set(_GTEST_LIBRARY_DIR "${GTEST_PREFIX}/lib")
  else()
    set(_GTEST_IMPORTED_TYPE IMPORTED_LOCATION)
    set(_GTEST_LIBRARY_SUFFIX
        "${CMAKE_GTEST_DEBUG_EXTENSION}${CMAKE_SHARED_LIBRARY_SUFFIX}")

    # Library and runtime same on non-Windows
    set(_GTEST_LIBRARY_DIR "${_GTEST_RUNTIME_DIR}")
  endif()

  set(GTEST_SHARED_LIB
      "${_GTEST_LIBRARY_DIR}/${CMAKE_SHARED_LIBRARY_PREFIX}gtest${_GTEST_LIBRARY_SUFFIX}")
  set(GMOCK_SHARED_LIB
      "${_GTEST_LIBRARY_DIR}/${CMAKE_SHARED_LIBRARY_PREFIX}gmock${_GTEST_LIBRARY_SUFFIX}")
  set(
    GTEST_MAIN_SHARED_LIB
    "${_GTEST_LIBRARY_DIR}/${CMAKE_SHARED_LIBRARY_PREFIX}gtest_main${_GTEST_LIBRARY_SUFFIX}"
    )
  set(GTEST_CMAKE_ARGS
      ${EP_COMMON_TOOLCHAIN}
      -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
      "-DCMAKE_INSTALL_PREFIX=${GTEST_PREFIX}"
      -DBUILD_SHARED_LIBS=ON
      -DCMAKE_CXX_FLAGS=${GTEST_CMAKE_CXX_FLAGS}
      -DCMAKE_CXX_FLAGS_${UPPERCASE_BUILD_TYPE}=${GTEST_CMAKE_CXX_FLAGS})
  set(GMOCK_INCLUDE_DIR "${GTEST_PREFIX}/include")

  if(APPLE)
    set(GTEST_CMAKE_ARGS ${GTEST_CMAKE_ARGS} "-DCMAKE_MACOSX_RPATH:BOOL=ON")
  endif()

  if(CMAKE_GENERATOR STREQUAL "Xcode")
    # Xcode projects support multi-configuration builds.  This forces the gtest build
    # to use the same output directory as a single-configuration Makefile driven build.
    list(
      APPEND GTEST_CMAKE_ARGS "-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=${_GTEST_LIBRARY_DIR}"
             "-DCMAKE_LIBRARY_OUTPUT_DIRECTORY_${CMAKE_BUILD_TYPE}=${_GTEST_RUNTIME_DIR}")
  endif()

  if(MSVC)
    if(NOT ("${CMAKE_GENERATOR}" STREQUAL "Ninja"))
      set(_GTEST_RUNTIME_DIR ${_GTEST_RUNTIME_DIR}/${CMAKE_BUILD_TYPE})
    endif()
    set(GTEST_CMAKE_ARGS
        ${GTEST_CMAKE_ARGS} "-DCMAKE_RUNTIME_OUTPUT_DIRECTORY=${_GTEST_RUNTIME_DIR}"
        "-DCMAKE_RUNTIME_OUTPUT_DIRECTORY_${CMAKE_BUILD_TYPE}=${_GTEST_RUNTIME_DIR}")
  else()
    list(
      APPEND GTEST_CMAKE_ARGS "-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=${_GTEST_RUNTIME_DIR}"
             "-DCMAKE_RUNTIME_OUTPUT_DIRECTORY_${CMAKE_BUILD_TYPE}=${_GTEST_RUNTIME_DIR}")
  endif()

  add_definitions(-DGTEST_LINKED_AS_SHARED_LIBRARY=1)

  if(MSVC AND NOT PEGASUS_USE_STATIC_CRT)
    set(GTEST_CMAKE_ARGS ${GTEST_CMAKE_ARGS} -Dgtest_force_shared_crt=ON)
  endif()

  externalproject_add(googletest_ep
                      URL ${GTEST_SOURCE_URL}
                      BUILD_BYPRODUCTS ${GTEST_SHARED_LIB} ${GTEST_MAIN_SHARED_LIB}
                                       ${GMOCK_SHARED_LIB}
                      CMAKE_ARGS ${GTEST_CMAKE_ARGS} ${EP_LOG_OPTIONS})

  # The include directory must exist before it is referenced by a target.
  file(MAKE_DIRECTORY "${GTEST_INCLUDE_DIR}")

  add_library(GTest::GTest SHARED IMPORTED)
  set_target_properties(GTest::GTest
                        PROPERTIES ${_GTEST_IMPORTED_TYPE} "${GTEST_SHARED_LIB}"
                                   INTERFACE_INCLUDE_DIRECTORIES "${GTEST_INCLUDE_DIR}")

  add_library(GTest::Main SHARED IMPORTED)
  set_target_properties(GTest::Main
                        PROPERTIES ${_GTEST_IMPORTED_TYPE} "${GTEST_MAIN_SHARED_LIB}"
                                   INTERFACE_INCLUDE_DIRECTORIES "${GTEST_INCLUDE_DIR}")

  add_library(GTest::GMock SHARED IMPORTED)
  set_target_properties(GTest::GMock
                        PROPERTIES ${_GTEST_IMPORTED_TYPE} "${GMOCK_SHARED_LIB}"
                                   INTERFACE_INCLUDE_DIRECTORIES "${GTEST_INCLUDE_DIR}")
  add_dependencies(toolchain-tests googletest_ep)
  add_dependencies(GTest::GTest googletest_ep)
  add_dependencies(GTest::Main googletest_ep)
  add_dependencies(GTest::GMock googletest_ep)
endmacro()

if(PEGASUS_BUILD_TESTS)
  resolve_dependency(GTest)

  if(NOT GTEST_VENDORED)
    # TODO(wesm): This logic does not work correctly with the MSVC static libraries
    # built for the shared crt

    #     set(CMAKE_REQUIRED_LIBRARIES GTest::GTest GTest::Main GTest::GMock)
    #     CHECK_CXX_SOURCE_COMPILES("
    # #include <gmock/gmock.h>
    # #include <gtest/gtest.h>

    # class A {
    #   public:
    #     int run() const { return 1; }
    # };

    # class B : public A {
    #   public:
    #     MOCK_CONST_METHOD0(run, int());
    # };

    # TEST(Base, Test) {
    #   B b;
    # }" GTEST_COMPILES_WITHOUT_MACRO)
    #     if (NOT GTEST_COMPILES_WITHOUT_MACRO)
    #       message(STATUS "Setting GTEST_LINKED_AS_SHARED_LIBRARY=1 on GTest::GTest")
    #       add_compile_definitions("GTEST_LINKED_AS_SHARED_LIBRARY=1")
    #     endif()
    #     set(CMAKE_REQUIRED_LIBRARIES)
  endif()

  get_target_property(GTEST_INCLUDE_DIR GTest::GTest INTERFACE_INCLUDE_DIRECTORIES)
  # TODO: Don't use global includes but rather target_include_directories
  include_directories(SYSTEM ${GTEST_INCLUDE_DIR})
endif()


if(PEGASUS_WITH_GRPC)
  if(c-ares_SOURCE STREQUAL "AUTO")
    find_package(c-ares QUIET CONFIG)
    if(NOT c-ares_FOUND)
      build_cares()
    endif()
  elseif(c-ares_SOURCE STREQUAL "BUNDLED")
    build_cares()
  elseif(c-ares_SOURCE STREQUAL "SYSTEM")
    find_package(c-ares REQUIRED CONFIG)
  endif()

  # TODO: Don't use global includes but rather target_include_directories
  get_target_property(CARES_INCLUDE_DIR c-ares::cares INTERFACE_INCLUDE_DIRECTORIES)
  include_directories(SYSTEM ${CARES_INCLUDE_DIR})
endif()

# ----------------------------------------------------------------------
# Dependencies for Arrow Flight RPC

macro(build_grpc)
  message(STATUS "Building gRPC from source")
  set(GRPC_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/grpc_ep-prefix/src/grpc_ep-build")
  set(GRPC_PREFIX "${THIRDPARTY_DIR}/grpc_ep-install")
  set(GRPC_HOME "${GRPC_PREFIX}")
  set(GRPC_INCLUDE_DIR "${GRPC_PREFIX}/include")
  set(GRPC_CMAKE_ARGS ${EP_COMMON_CMAKE_ARGS} "-DCMAKE_INSTALL_PREFIX=${GRPC_PREFIX}"
                      -DBUILD_SHARED_LIBS=OFF)

  set(
    GRPC_STATIC_LIBRARY_GPR
    "${GRPC_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}gpr${CMAKE_STATIC_LIBRARY_SUFFIX}")
  set(
    GRPC_STATIC_LIBRARY_GRPC
    "${GRPC_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}grpc${CMAKE_STATIC_LIBRARY_SUFFIX}")
  set(
    GRPC_STATIC_LIBRARY_GRPCPP
    "${GRPC_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}grpc++${CMAKE_STATIC_LIBRARY_SUFFIX}"
    )
  set(
    GRPC_STATIC_LIBRARY_ADDRESS_SORTING
    "${GRPC_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}address_sorting${CMAKE_STATIC_LIBRARY_SUFFIX}"
    )
  set(GRPC_CPP_PLUGIN "${GRPC_PREFIX}/bin/grpc_cpp_plugin")

  set(GRPC_CMAKE_PREFIX)

  add_custom_target(grpc_dependencies)

  if(CARES_VENDORED)
    add_dependencies(grpc_dependencies cares_ep)
  endif()

  if(GFLAGS_VENDORED)
    add_dependencies(grpc_dependencies gflags_ep)
  endif()

  add_dependencies(grpc_dependencies protobuf::libprotobuf c-ares::cares)

  get_target_property(GRPC_PROTOBUF_INCLUDE_DIR protobuf::libprotobuf
                      INTERFACE_INCLUDE_DIRECTORIES)
  get_filename_component(GRPC_PB_ROOT "${GRPC_PROTOBUF_INCLUDE_DIR}" DIRECTORY)
  get_target_property(GRPC_Protobuf_PROTOC_LIBRARY protobuf::libprotoc IMPORTED_LOCATION)
  get_target_property(GRPC_CARES_INCLUDE_DIR c-ares::cares INTERFACE_INCLUDE_DIRECTORIES)
  get_filename_component(GRPC_CARES_ROOT "${GRPC_CARES_INCLUDE_DIR}" DIRECTORY)
  get_target_property(GRPC_GFLAGS_INCLUDE_DIR ${GFLAGS_LIBRARIES}
                      INTERFACE_INCLUDE_DIRECTORIES)
  get_filename_component(GRPC_GFLAGS_ROOT "${GRPC_GFLAGS_INCLUDE_DIR}" DIRECTORY)

  set(GRPC_CMAKE_PREFIX "${GRPC_CMAKE_PREFIX};${GRPC_PB_ROOT}")
  set(GRPC_CMAKE_PREFIX "${GRPC_CMAKE_PREFIX};${GRPC_GFLAGS_ROOT}")
  set(GRPC_CMAKE_PREFIX "${GRPC_CMAKE_PREFIX};${GRPC_CARES_ROOT}")

  # ZLIB is never vendored
  set(GRPC_CMAKE_PREFIX "${GRPC_CMAKE_PREFIX};${ZLIB_ROOT}")

  if(RAPIDJSON_VENDORED)
    add_dependencies(grpc_dependencies rapidjson_ep)
  endif()

  # Yuck, see https://stackoverflow.com/a/45433229/776560
  string(REPLACE ";" "|" GRPC_PREFIX_PATH_ALT_SEP "${GRPC_CMAKE_PREFIX}")

  set(GRPC_CMAKE_ARGS
      -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
      -DCMAKE_PREFIX_PATH='${GRPC_PREFIX_PATH_ALT_SEP}'
      -DgRPC_CARES_PROVIDER=package
      -DgRPC_GFLAGS_PROVIDER=package
      -DgRPC_PROTOBUF_PROVIDER=package
      -DgRPC_SSL_PROVIDER=package
      -DgRPC_ZLIB_PROVIDER=package
      -DCMAKE_CXX_FLAGS=${EP_CXX_FLAGS}
      -DCMAKE_C_FLAGS=${EP_C_FLAGS}
      -DCMAKE_INSTALL_PREFIX=${GRPC_PREFIX}
      -DCMAKE_INSTALL_LIBDIR=lib
      "-DProtobuf_PROTOC_LIBRARY=${GRPC_Protobuf_PROTOC_LIBRARY}"
      -DBUILD_SHARED_LIBS=OFF)
  if(OPENSSL_ROOT_DIR)
    list(APPEND GRPC_CMAKE_ARGS -DOPENSSL_ROOT_DIR=${OPENSSL_ROOT_DIR})
  endif()

  # XXX the gRPC git checkout is huge and takes a long time
  # Ideally, we should be able to use the tarballs, but they don't contain
  # vendored dependencies such as c-ares...
  externalproject_add(grpc_ep
                      URL ${GRPC_SOURCE_URL}
                      LIST_SEPARATOR |
                      BUILD_BYPRODUCTS ${GRPC_STATIC_LIBRARY_GPR}
                                       ${GRPC_STATIC_LIBRARY_GRPC}
                                       ${GRPC_STATIC_LIBRARY_GRPCPP}
                                       ${GRPC_STATIC_LIBRARY_ADDRESS_SORTING}
                                       ${GRPC_CPP_PLUGIN}
                      CMAKE_ARGS ${GRPC_CMAKE_ARGS} ${EP_LOG_OPTIONS}
                      DEPENDS ${grpc_dependencies})

  # Work around https://gitlab.kitware.com/cmake/cmake/issues/15052
  file(MAKE_DIRECTORY ${GRPC_INCLUDE_DIR})

  add_library(gRPC::gpr STATIC IMPORTED)
  set_target_properties(gRPC::gpr
                        PROPERTIES IMPORTED_LOCATION "${GRPC_STATIC_LIBRARY_GPR}"
                                   INTERFACE_INCLUDE_DIRECTORIES "${GRPC_INCLUDE_DIR}")

  add_library(gRPC::grpc STATIC IMPORTED)
  set_target_properties(gRPC::grpc
                        PROPERTIES IMPORTED_LOCATION "${GRPC_STATIC_LIBRARY_GRPC}"
                                   INTERFACE_INCLUDE_DIRECTORIES "${GRPC_INCLUDE_DIR}")

  add_library(gRPC::grpc++ STATIC IMPORTED)
  set_target_properties(gRPC::grpc++
                        PROPERTIES IMPORTED_LOCATION "${GRPC_STATIC_LIBRARY_GRPCPP}"
                                   INTERFACE_INCLUDE_DIRECTORIES "${GRPC_INCLUDE_DIR}")

  add_library(gRPC::address_sorting STATIC IMPORTED)
  set_target_properties(gRPC::address_sorting
                        PROPERTIES IMPORTED_LOCATION
                                   "${GRPC_STATIC_LIBRARY_ADDRESS_SORTING}"
                                   INTERFACE_INCLUDE_DIRECTORIES "${GRPC_INCLUDE_DIR}")

  add_executable(gRPC::grpc_cpp_plugin IMPORTED)
  set_target_properties(gRPC::grpc_cpp_plugin
                        PROPERTIES IMPORTED_LOCATION ${GRPC_CPP_PLUGIN})

  add_dependencies(grpc_ep grpc_dependencies)
  add_dependencies(toolchain grpc_ep)
  add_dependencies(gRPC::gpr grpc_ep)
  add_dependencies(gRPC::grpc grpc_ep)
  add_dependencies(gRPC::grpc++ grpc_ep)
  add_dependencies(gRPC::address_sorting grpc_ep)
  set(GRPC_VENDORED TRUE)
endmacro()

if(PEGASUS_WITH_GRPC)
  set(PEGASUS_GRPC_REQUIRED_VERSION "1.17.0")
  if(gRPC_SOURCE STREQUAL "AUTO")
    find_package(gRPC ${PEGASUS_GRPC_REQUIRED_VERSION} QUIET)
    if(NOT gRPC_FOUND)
      # Ubuntu doesn't package the CMake config
      find_package(gRPCAlt ${PEGASUS_GRPC_REQUIRED_VERSION})
    endif()
    if(NOT gRPC_FOUND AND NOT gRPCAlt_FOUND)
      build_grpc()
    endif()
  elseif(gRPC_SOURCE STREQUAL "BUNDLED")
    build_grpc()
  elseif(gRPC_SOURCE STREQUAL "SYSTEM")
    find_package(gRPC ${PEGASUS_GRPC_REQUIRED_VERSION} QUIET)
    if(NOT gRPC_FOUND)
      # Ubuntu doesn't package the CMake config
      find_package(gRPCAlt ${PEGASUS_GRPC_REQUIRED_VERSION} REQUIRED)
    endif()
  endif()

  get_target_property(GRPC_CPP_PLUGIN gRPC::grpc_cpp_plugin IMPORTED_LOCATION)
  if(NOT GRPC_CPP_PLUGIN)
    get_target_property(GRPC_CPP_PLUGIN gRPC::grpc_cpp_plugin IMPORTED_LOCATION_RELEASE)
  endif()

  if(TARGET gRPC::address_sorting)
    set(GRPC_HAS_ADDRESS_SORTING TRUE)
  else()
    set(GRPC_HAS_ADDRESS_SORTING FALSE)
  endif()

  # TODO: Don't use global includes but rather target_include_directories
  get_target_property(GRPC_INCLUDE_DIR gRPC::grpc INTERFACE_INCLUDE_DIRECTORIES)
  include_directories(SYSTEM ${GRPC_INCLUDE_DIR})

  if(GRPC_VENDORED)
    set(GRPCPP_PP_INCLUDE TRUE)
  else()
    # grpc++ headers may reside in ${GRPC_INCLUDE_DIR}/grpc++ or ${GRPC_INCLUDE_DIR}/grpcpp
    # depending on the gRPC version.
    if(EXISTS "${GRPC_INCLUDE_DIR}/grpcpp/impl/codegen/config_protobuf.h")
      set(GRPCPP_PP_INCLUDE TRUE)
    elseif(EXISTS "${GRPC_INCLUDE_DIR}/grpc++/impl/codegen/config_protobuf.h")
      set(GRPCPP_PP_INCLUDE FALSE)
    else()
      message(FATAL_ERROR "Cannot find grpc++ headers in ${GRPC_INCLUDE_DIR}")
    endif()
  endif()
endif()

macro(build_arrow)
  message(STATUS "Building Arrow from source")

  set(ARROW_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/arrow_ep-prefix/src/arrow_ep")
  set(ARROW_INCLUDE_DIR "${ARROW_PREFIX}/include")

  set(ARROW_LIB_DIR "${ARROW_PREFIX}")
  if (MSVC)
    set(ARROW_SHARED_LIB "${ARROW_PREFIX}/bin/arrow.dll")
    set(ARROW_SHARED_IMPLIB "${ARROW_LIB_DIR}/arrow.lib")
    set(ARROW_STATIC_LIB "${ARROW_LIB_DIR}/arrow_static.lib")
  else()
    set(ARROW_SHARED_LIB "${ARROW_LIB_DIR}/libarrow${CMAKE_SHARED_LIBRARY_SUFFIX}")
    set(ARROW_STATIC_LIB "${ARROW_LIB_DIR}/libarrow.a")
  endif()
  
  set(ARROW_CMAKE_ARGS
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    -DCMAKE_CXX_FLAGS=${EP_CXX_FLAGS}
    -DCMAKE_C_FLAGS=${EP_C_FLAGS}
    -DCMAKE_INSTALL_PREFIX=${ARROW_PREFIX}
    -DCMAKE_INSTALL_LIBDIR=${ARROW_LIB_DIR}
    -DARROW_JEMALLOC=OFF
    -DARROW_PARQUET=ON
    -DARROW_FLIGHT=ON
    -DARROW_HDFS=ON
    -DARROW_BUILD_SHARED=${PEGASUS_BUILD_SHARED}
    -DARROW_BOOST_USE_SHARED=${PEGASUS_BOOST_USE_SHARED}
    -DARROW_BUILD_TESTS=OFF)
  
  if (MSVC AND PARQUET_USE_STATIC_CRT)
    set(ARROW_CMAKE_ARGS ${ARROW_CMAKE_ARGS} -DARROW_USE_STATIC_CRT=ON)
  endif()
  
  if ("$ENV{PEGASUS_ARROW_VERSION}" STREQUAL "")
    set(ARROW_VERSION "apache-arrow-0.15.1")
  else()
    set(ARROW_VERSION "$ENV{PEGASUS_ARROW_VERSION}")
  endif()
  message(STATUS "Building Apache Arrow from commit: ${ARROW_VERSION}")
  
  set(ARROW_URL "https://github.com/apache/arrow/archive/${ARROW_VERSION}.tar.gz")
  
  if (CMAKE_VERSION VERSION_GREATER "3.7")
    set(ARROW_CONFIGURE SOURCE_SUBDIR "cpp" CMAKE_ARGS ${ARROW_CMAKE_ARGS})
  else()
    set(ARROW_CONFIGURE CONFIGURE_COMMAND "${CMAKE_COMMAND}" -G "${CMAKE_GENERATOR}"
      ${ARROW_CMAKE_ARGS} "${CMAKE_CURRENT_BINARY_DIR}/arrow_ep-prefix/src/arrow_ep/cpp")
  endif()
  
  ExternalProject_Add(arrow_ep
    URL ${ARROW_URL}
    ${ARROW_CONFIGURE}
    BUILD_BYPRODUCTS "${ARROW_SHARED_LIB}" "${ARROW_STATIC_LIB}")
  
  if (MSVC)
    ExternalProject_Add_Step(arrow_ep copy_dll_step
      DEPENDEES install
      COMMAND ${CMAKE_COMMAND} -E make_directory ${BUILD_OUTPUT_ROOT_DIRECTORY}
      COMMAND ${CMAKE_COMMAND} -E copy ${ARROW_SHARED_LIB} ${BUILD_OUTPUT_ROOT_DIRECTORY})
  endif()
endmacro()

set(PEGASUS_NEED_ARROW 1)

if(PEGASUS_NEED_ARROW)
  find_package(Arrow)
  if(NOT ARROW_FOUND)
    build_arrow()
    set(ARROW_VENDORED 1)
  else()
    set(ARROW_VENDORED 0)
  endif()

  include_directories(SYSTEM ${ARROW_INCLUDE_DIR})
  add_library(arrow SHARED IMPORTED)
  if(MSVC)
    set_target_properties(arrow
                        PROPERTIES IMPORTED_IMPLIB "${ARROW_SHARED_IMPLIB}")
  else()
    set_target_properties(arrow
                        PROPERTIES IMPORTED_LOCATION "${ARROW_SHARED_LIB}")
  endif()
  add_library(arrow_static STATIC IMPORTED)
  set_target_properties(arrow_static PROPERTIES IMPORTED_LOCATION ${ARROW_STATIC_LIB})

  if (ARROW_VENDORED)
    add_dependencies(arrow arrow_ep)
    add_dependencies(arrow_static arrow_ep)
  endif()
endif()

set_target_properties(arrow PROPERTIES IMPORTED_LOCATION "${ARROW_SHARED_LIB}")
