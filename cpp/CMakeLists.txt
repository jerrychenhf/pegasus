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

cmake_minimum_required(VERSION 3.2)
message(STATUS "Building using CMake version: ${CMAKE_VERSION}")

set(PEGASUS_VERSION "1.0.0-SNAPSHOT")

string(REGEX MATCH "^[0-9]+\\.[0-9]+\\.[0-9]+" PEGASUS_BASE_VERSION "${PEGASUS_VERSION}")

# if no build build type is specified, default to release builds
if(NOT DEFINED CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build.")
endif()
string(TOLOWER ${CMAKE_BUILD_TYPE} LOWERCASE_BUILD_TYPE)
string(TOUPPER ${CMAKE_BUILD_TYPE} UPPERCASE_BUILD_TYPE)

project(pegasus VERSION "${PEGASUS_BASE_VERSION}")

set(PEGASUS_VERSION_MAJOR "${pegasus_VERSION_MAJOR}")
set(PEGASUS_VERSION_MINOR "${pegasus_VERSION_MINOR}")
set(PEGASUS_VERSION_PATCH "${pegasus_VERSION_PATCH}")
if(PEGASUS_VERSION_MAJOR STREQUAL ""
   OR PEGASUS_VERSION_MINOR STREQUAL ""
   OR PEGASUS_VERSION_PATCH STREQUAL "")
  message(FATAL_ERROR "Failed to determine Pegasus version from '${PEGASUS_VERSION}'")
endif()

message(STATUS "Pegasus version: "
               "${PEGASUS_VERSION_MAJOR}.${PEGASUS_VERSION_MINOR}.${PEGASUS_VERSION_PATCH} "
               "(full: '${PEGASUS_VERSION}')")

set(PEGASUS_SOURCE_DIR ${PROJECT_SOURCE_DIR})
set(PEGASUS_BINARY_DIR ${PROJECT_BINARY_DIR})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules")

include(cmake_modules/cmake_fns.txt)
include(FindPegasus)
include(FindArrow)
include(FindArrowFlight)

include(CMakePackageConfigHelpers)
include(CMakeParseArguments)
include(ExternalProject)
include(FindPkgConfig)

include(GNUInstallDirs)

cmake_policy(SET CMP0025 NEW)

set(BUILD_SUPPORT_DIR "${CMAKE_SOURCE_DIR}/build-support")

set(PEGASUS_CMAKE_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")


find_package(ClangTools)
if("$ENV{CMAKE_EXPORT_COMPILE_COMMANDS}" STREQUAL "1" OR CLANG_TIDY_FOUND)
  # Generate a Clang compile_commands.json "compilation database" file for use
  # with various development tools, such as Vim's YouCompleteMe plugin.
  # See http://clang.llvm.org/docs/JSONCompilationDatabase.html
  set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
endif()

find_package(InferTools)
if("$ENV{CMAKE_EXPORT_COMPILE_COMMANDS}" STREQUAL "1" OR INFER_FOUND)
  # Generate a Clang compile_commands.json "compilation database" file for use
  # with various development tools, such as Vim's YouCompleteMe plugin.
  # See http://clang.llvm.org/docs/JSONCompilationDatabase.html
  set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
endif()

# ----------------------------------------------------------------------
# cmake options
include(DefineOptions)

# Needed for linting targets, etc.
find_package(PythonInterp)

if(PEGASUS_USE_CCACHE)
  find_program(CCACHE_FOUND ccache)
  if(CCACHE_FOUND)
    message(STATUS "Using ccache: ${CCACHE_FOUND}")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ${CCACHE_FOUND})
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ${CCACHE_FOUND})
    set(ENV{CCACHE_COMMENTS} "1")
  endif(CCACHE_FOUND)
endif()

#
# "make lint" target
#
set(PEGASUS_LINT_QUIET "--quiet")


if(NOT LINT_EXCLUSIONS_FILE)
  # source files matching a glob from a line in this file
  # will be excluded from linting (cpplint, clang-tidy, clang-format)
  set(LINT_EXCLUSIONS_FILE ${BUILD_SUPPORT_DIR}/lint_exclusions.txt)
endif()

find_program(CPPLINT_BIN NAMES cpplint cpplint.py HINTS ${BUILD_SUPPORT_DIR})
message(STATUS "Found cpplint executable at ${CPPLINT_BIN}")

add_custom_target(lint
                  ${PYTHON_EXECUTABLE}
                  ${BUILD_SUPPORT_DIR}/run_cpplint.py
                  --cpplint_binary
                  ${CPPLINT_BIN}
                  --exclude_globs
                  ${LINT_EXCLUSIONS_FILE}
                  --source_dir
                  ${CMAKE_CURRENT_SOURCE_DIR}/src
                  ${PEGASUS_LINT_QUIET})

#
# "make format" and "make check-format" targets
#
if(${CLANG_FORMAT_FOUND})
  # runs clang format and updates files in place.
  add_custom_target(format
                    ${PYTHON_EXECUTABLE}
                    ${BUILD_SUPPORT_DIR}/run_clang_format.py
                    --clang_format_binary
                    ${CLANG_FORMAT_BIN}
                    --exclude_globs
                    ${LINT_EXCLUSIONS_FILE}
                    --source_dir
                    ${CMAKE_CURRENT_SOURCE_DIR}/src
                    --fix
                    ${PEGASUS_LINT_QUIET})

  # runs clang format and exits with a non-zero exit code if any files need to be reformatted
  add_custom_target(check-format
                    ${PYTHON_EXECUTABLE}
                    ${BUILD_SUPPORT_DIR}/run_clang_format.py
                    --clang_format_binary
                    ${CLANG_FORMAT_BIN}
                    --exclude_globs
                    ${LINT_EXCLUSIONS_FILE}
                    --source_dir
                    ${CMAKE_CURRENT_SOURCE_DIR}/src
                    ${PEGASUS_LINT_QUIET})
endif()

add_custom_target(lint_cpp_cli ${PYTHON_EXECUTABLE} ${BUILD_SUPPORT_DIR}/lint_cpp_cli.py
                  ${CMAKE_CURRENT_SOURCE_DIR}/src)


if(UNIX)
  add_custom_target(iwyu ${BUILD_SUPPORT_DIR}/iwyu/iwyu.sh)
endif(UNIX)

#
# Set up various options
#

set(PEGASUS_IPC ON)

if(NOT PEGASUS_BUILD_TESTS)
  set(NO_TESTS 1)
else()
  add_custom_target(all-tests)
  add_custom_target(unittest
                    ctest
                    -j4
                    -L
                    unittest
                    --output-on-failure)
  add_dependencies(unittest all-tests)
endif()

#
# Compiler flags
#

# Determine compiler version
include(CompilerInfo)

include(SetupCxxFlags)

#
# Build output directory
#

# set compile output directory
string(TOLOWER ${CMAKE_BUILD_TYPE} BUILD_SUBDIR_NAME)

# If build in-source, create the latest symlink. If build out-of-source, which is
# preferred, simply output the binaries in the build folder
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_BINARY_DIR})
  set(BUILD_OUTPUT_ROOT_DIRECTORY
      "${CMAKE_CURRENT_BINARY_DIR}/build/${BUILD_SUBDIR_NAME}/")
  # Link build/latest to the current build directory, to avoid developers
  # accidentally running the latest debug build when in fact they're building
  # release builds.
  file(MAKE_DIRECTORY ${BUILD_OUTPUT_ROOT_DIRECTORY})
  if(NOT APPLE)
    set(MORE_ARGS "-T")
  endif()
  execute_process(COMMAND ln
                          ${MORE_ARGS}
                          -sf
                          ${BUILD_OUTPUT_ROOT_DIRECTORY}
                          ${CMAKE_CURRENT_BINARY_DIR}/build/latest)
else()
  set(BUILD_OUTPUT_ROOT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${BUILD_SUBDIR_NAME}/")
endif()

# where to put generated archives (.a files)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${BUILD_OUTPUT_ROOT_DIRECTORY}")
set(ARCHIVE_OUTPUT_DIRECTORY "${BUILD_OUTPUT_ROOT_DIRECTORY}")

# where to put generated libraries (.so files)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${BUILD_OUTPUT_ROOT_DIRECTORY}")
set(LIBRARY_OUTPUT_DIRECTORY "${BUILD_OUTPUT_ROOT_DIRECTORY}")

# where to put generated binaries
set(EXECUTABLE_OUTPUT_PATH "${BUILD_OUTPUT_ROOT_DIRECTORY}")

if(CMAKE_GENERATOR STREQUAL Xcode)
  # Xcode projects support multi-configuration builds.  This forces a single output directory
  # when building with Xcode that is consistent with single-configuration Makefile driven build.
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${UPPERCASE_BUILD_TYPE}
      "${BUILD_OUTPUT_ROOT_DIRECTORY}")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${UPPERCASE_BUILD_TYPE}
      "${BUILD_OUTPUT_ROOT_DIRECTORY}")
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${UPPERCASE_BUILD_TYPE}
      "${BUILD_OUTPUT_ROOT_DIRECTORY}")
endif()

#
# Dependencies
#

include(BuildUtils)
enable_testing()

include(ThirdpartyToolchain)

# Add common flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX_COMMON_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${PEGASUS_CXXFLAGS}")

# For any C code, use the same flags. These flags don't contain
# C++ specific flags.
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${PEGASUS_CXX_FLAGS} ${CXX_COMMON_FLAGS}")

# Remove --std=c++11 to avoid errors from C compilers
string(REPLACE "-std=c++11" "" CMAKE_C_FLAGS ${CMAKE_C_FLAGS})

# Add C++-only flags, like -std=c++11
set(CMAKE_CXX_FLAGS "${CXX_ONLY_FLAGS} ${CMAKE_CXX_FLAGS}")

# CMAKE_CXX_FLAGS now fully assembled
message(STATUS "CMAKE_C_FLAGS: ${CMAKE_C_FLAGS}")
message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")

include_directories(${CMAKE_CURRENT_BINARY_DIR}/src)
include_directories(src)

# Compiled flatbuffers files
include_directories(src/generated)

#
# Visibility
#
# For generate_export_header() and add_compiler_export_flags().
include(GenerateExportHeader)

if(PARQUET_BUILD_SHARED)
  if(POLICY CMP0063)
    set_target_properties(pegasus_shared
                          PROPERTIES C_VISIBILITY_PRESET
                                     hidden
                                     CXX_VISIBILITY_PRESET
                                     hidden
                                     VISIBILITY_INLINES_HIDDEN
                                     1)
  else()
    # Sets -fvisibility=hidden for gcc
    add_compiler_export_flags()
  endif()
endif()

#
# "make ctags" target
#
if(UNIX)
  add_custom_target(ctags ctags -R --languages=c++,c)
endif(UNIX)

#
# "make etags" target
#
if(UNIX)
  add_custom_target(tags
                    etags
                    --members
                    --declarations
                    `find
                    ${CMAKE_CURRENT_SOURCE_DIR}/src
                    -name
                    \\*.cc
                    -or
                    -name
                    \\*.hh
                    -or
                    -name
                    \\*.cpp
                    -or
                    -name
                    \\*.h
                    -or
                    -name
                    \\*.c
                    -or
                    -name
                    \\*.f`)
  add_custom_target(etags DEPENDS tags)
endif(UNIX)

#
# "make cscope" target
#
if(UNIX)
  add_custom_target(cscope find ${CMAKE_CURRENT_SOURCE_DIR}
                    (-name
                     \\*.cc
                     -or
                     -name
                     \\*.hh
                     -or
                     -name
                     \\*.cpp
                     -or
                     -name
                     \\*.h
                     -or
                     -name
                     \\*.c
                     -or
                     -name
                     \\*.f)
                    -exec
                    echo
                    \"{}\"
                    \;
                    >
                    cscope.files
                    &&
                    cscope
                    -q
                    -b
                    VERBATIM)
endif(UNIX)

#
# "make infer" target
#

if(${INFER_FOUND})
  # runs infer capture
  add_custom_target(infer
                    ${BUILD_SUPPORT_DIR}/run-infer.sh
                    ${INFER_BIN}
                    ${CMAKE_BINARY_DIR}/compile_commands.json
                    1)
  # runs infer analyze
  add_custom_target(infer-analyze
                    ${BUILD_SUPPORT_DIR}/run-infer.sh
                    ${INFER_BIN}
                    ${CMAKE_BINARY_DIR}/compile_commands.json
                    2)
  # runs infer report
  add_custom_target(infer-report
                    ${BUILD_SUPPORT_DIR}/run-infer.sh
                    ${INFER_BIN}
                    ${CMAKE_BINARY_DIR}/compile_commands.json
                    3)
endif()

#
# Linker and Dependencies
#

if(PEGASUS_WITH_URIPARSER)
  list(APPEND PEGASUS_STATIC_LINK_LIBS uriparser::uriparser)
  list(APPEND PEGASUS_STATIC_INSTALL_INTERFACE_LIBS uriparser::uriparser)
endif()

if(PEGASUS_USE_OPENSSL)
  set(PEGASUS_OPENSSL_LIBS OpenSSL::Crypto OpenSSL::SSL)
  list(APPEND PEGASUS_LINK_LIBS ${PEGASUS_OPENSSL_LIBS})
  list(APPEND PEGASUS_STATIC_LINK_LIBS ${PEGASUS_OPENSSL_LIBS})
  list(APPEND PEGASUS_STATIC_INSTALL_INTERFACE_LIBS ${PEGASUS_OPENSSL_LIBS})
endif()

if(PEGASUS_WITH_SNAPPY)
  list(APPEND PEGASUS_STATIC_LINK_LIBS Snappy::snappy)
  list(APPEND PEGASUS_STATIC_INSTALL_INTERFACE_LIBS Snappy::snappy)
endif()

if(PEGASUS_USE_GLOG)
  list(APPEND PEGASUS_LINK_LIBS GLOG::glog)
  list(APPEND PEGASUS_STATIC_LINK_LIBS GLOG::glog)
  list(APPEND PEGASUS_STATIC_INSTALL_INTERFACE_LIBS GLOG::glog)
  add_definitions("-DPEGASUS_USE_GLOG")
endif()

add_custom_target(pegasus_dependencies)
add_custom_target(pegasus_benchmark_dependencies)
add_custom_target(pegasus_test_dependencies)

# PEGASUS-4581: CMake can be finicky about invoking the ExternalProject builds
# for some of the library dependencies, so we "nuke it from orbit" by making
# the toolchain dependency explicit using these "dependencies" targets
add_dependencies(pegasus_dependencies toolchain)
add_dependencies(pegasus_test_dependencies toolchain-tests)

if(PEGASUS_STATIC_LINK_LIBS)
  add_dependencies(pegasus_dependencies ${PEGASUS_STATIC_LINK_LIBS})
endif()

set(PEGASUS_SHARED_PRIVATE_LINK_LIBS ${PEGASUS_STATIC_LINK_LIBS})

if(NOT MSVC)
  list(APPEND PEGASUS_LINK_LIBS ${CMAKE_DL_LIBS})
  list(APPEND PEGASUS_SHARED_INSTALL_INTERFACE_LIBS ${CMAKE_DL_LIBS})
endif()

set(PEGASUS_TEST_LINK_TOOLCHAIN GTest::Main GTest::GTest GTest::GMock)

if(PEGASUS_BUILD_TESTS)
  add_dependencies(pegasus_test_dependencies ${PEGASUS_TEST_LINK_TOOLCHAIN})
endif()


set(PEGASUS_TEST_STATIC_LINK_LIBS pegasus_testing_static pegasus_static ${PEGASUS_LINK_LIBS}
                                ${PEGASUS_TEST_LINK_TOOLCHAIN})

set(PEGASUS_TEST_SHARED_LINK_LIBS
    pegasus_shared
    ${PEGASUS_LINK_LIBS}
    ${BOOST_FILESYSTEM_LIBRARY}
    ${BOOST_SYSTEM_LIBRARY}
    ${PEGASUS_TEST_LINK_TOOLCHAIN})

if(NOT MSVC)
  set(PEGASUS_TEST_SHARED_LINK_LIBS ${PEGASUS_TEST_SHARED_LINK_LIBS} ${CMAKE_DL_LIBS})
endif()

if("${PEGASUS_TEST_LINKAGE}" STREQUAL "shared")
  if(PEGASUS_BUILD_TESTS AND NOT PEGASUS_BUILD_SHARED)
    message(FATAL_ERROR "If using shared linkage for unit tests, must also \
pass PEGASUS_BUILD_SHARED=on")
  endif()
  # Use shared linking for unit tests if it's available
  set(PEGASUS_TEST_LINK_LIBS ${PEGASUS_TEST_SHARED_LINK_LIBS})
else()
  if(PEGASUS_BUILD_TESTS AND NOT PEGASUS_BUILD_STATIC)
    message(FATAL_ERROR "If using static linkage for unit tests, must also \
pass PEGASUS_BUILD_STATIC=on")
  endif()
  set(PEGASUS_TEST_LINK_LIBS ${PEGASUS_TEST_STATIC_LINK_LIBS})
endif()

set(PEGASUS_SYSTEM_LINK_LIBS)

if(PEGASUS_JEMALLOC)
  add_definitions(-DPEGASUS_JEMALLOC)
  add_definitions(-DPEGASUS_JEMALLOC_INCLUDE_DIR=${JEMALLOC_INCLUDE_DIR})
  list(APPEND PEGASUS_SYSTEM_LINK_LIBS jemalloc::jemalloc)
endif()

if(PEGASUS_MIMALLOC)
  add_definitions(-DPEGASUS_MIMALLOC)
  list(APPEND PEGASUS_SYSTEM_LINK_LIBS mimalloc::mimalloc)
endif()

if(THREADS_FOUND)
  list(APPEND PEGASUS_SYSTEM_LINK_LIBS Threads::Threads)
endif()

if(NOT WIN32 AND NOT APPLE)
  # Pass -lrt on Linux only
  list(APPEND PEGASUS_SYSTEM_LINK_LIBS rt)
endif()

list(APPEND PEGASUS_LINK_LIBS ${PEGASUS_SYSTEM_LINK_LIBS})
list(APPEND PEGASUS_STATIC_LINK_LIBS ${PEGASUS_SYSTEM_LINK_LIBS})
list(APPEND PEGASUS_STATIC_INSTALL_INTERFACE_LIBS ${PEGASUS_SYSTEM_LINK_LIBS})

#
# Subdirectories
#

add_subdirectory(src/pegasus)

config_summary_message()
if(${PEGASUS_BUILD_CONFIG_SUMMARY_JSON})
  config_summary_json()
endif()
