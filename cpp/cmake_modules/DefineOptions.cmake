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

macro(set_option_category name)
  set(PEGASUS_OPTION_CATEGORY ${name})
  list(APPEND "PEGASUS_OPTION_CATEGORIES" ${name})
endmacro()

macro(define_option name description default)
  option(${name} ${description} ${default})
  list(APPEND "PEGASUS_${PEGASUS_OPTION_CATEGORY}_OPTION_NAMES" ${name})
  set("${name}_OPTION_DESCRIPTION" ${description})
  set("${name}_OPTION_DEFAULT" ${default})
  set("${name}_OPTION_TYPE" "bool")
endmacro()

function(list_join lst glue out)
  if("${${lst}}" STREQUAL "")
    set(${out} "" PARENT_SCOPE)
    return()
  endif()

  list(GET ${lst} 0 joined)
  list(REMOVE_AT ${lst} 0)
  foreach(item ${${lst}})
    set(joined "${joined}${glue}${item}")
  endforeach()
  set(${out} ${joined} PARENT_SCOPE)
endfunction()

macro(define_option_string name description default)
  set(${name} ${default} CACHE STRING ${description})
  list(APPEND "PEGASUS_${PEGASUS_OPTION_CATEGORY}_OPTION_NAMES" ${name})
  set("${name}_OPTION_DESCRIPTION" ${description})
  set("${name}_OPTION_DEFAULT" "\"${default}\"")
  set("${name}_OPTION_TYPE" "string")

  set("${name}_OPTION_ENUM" ${ARGN})
  list_join("${name}_OPTION_ENUM" "|" "${name}_OPTION_ENUM")
  if(NOT ("${${name}_OPTION_ENUM}" STREQUAL ""))
    set_property(CACHE ${name} PROPERTY STRINGS ${ARGN})
  endif()
endmacro()

# Top level cmake dir
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
  #----------------------------------------------------------------------
  set_option_category("Compile and link")

  define_option_string(PEGASUS_CXXFLAGS "Compiler flags to append when compiling Pegasus" "")

  define_option(PEGASUS_BUILD_STATIC "Build static libraries" ON)

  define_option(PEGASUS_BUILD_SHARED "Build shared libraries" ON)

  define_option(PEGASUS_USE_CCACHE "Use ccache when compiling (if available)" ON)

  # Disable this option to exercise non-SIMD fallbacks
  define_option(PEGASUS_USE_SIMD "Build with SIMD optimizations" ON)

  define_option(PEGASUS_GGDB_DEBUG "Pass -ggdb flag to debug builds" ON)

  #----------------------------------------------------------------------
  set_option_category("Test and benchmark")

  define_option(PEGASUS_BUILD_TESTS "Build the Pegasus googletest unit tests" ON)

  define_option_string(PEGASUS_TEST_LINKAGE
                       "Linkage of Pegasus libraries with unit tests executables."
                       "shared"
                       "shared"
                       "static")

  #----------------------------------------------------------------------
  set_option_category("Thirdparty toolchain")


  if(NOT "$ENV{CONDA_PREFIX}" STREQUAL "")
    set(PEGASUS_DEPENDENCY_SOURCE_DEFAULT "CONDA")
  else()
    set(PEGASUS_DEPENDENCY_SOURCE_DEFAULT "AUTO")
  endif()
  define_option_string(PEGASUS_DEPENDENCY_SOURCE
                       "Method to use for acquiring pegasus's build dependencies"
                       "${PEGASUS_DEPENDENCY_SOURCE_DEFAULT}"
                       "AUTO"
                       "BUNDLED"
                       "SYSTEM"
                       "CONDA"
                       "BREW")

  define_option(PEGASUS_BOOST_USE_SHARED "Rely on boost shared libraries where relevant" ON)

  define_option(PEGASUS_PROTOBUF_USE_SHARED
                "Rely on Protocol Buffers shared libraries where relevant" ON)

  define_option(PEGASUS_GFLAGS_USE_SHARED "Rely on GFlags shared libraries where relevant"
                ON)

  define_option(PEGASUS_USE_GLOG "Build libraries with glog support for pluggable logging"
                OFF)

  define_option(PEGASUS_WITH_SNAPPY "Build with Snappy compression" OFF)


  #----------------------------------------------------------------------
  set_option_category("Parquet")

  define_option(PARQUET_MINIMAL_DEPENDENCY
                "Depend only on Thirdparty headers to build libparquet. \
Always OFF if building binaries" OFF)

  define_option(
    PARQUET_BUILD_EXECUTABLES
    "Build the Parquet executable CLI tools. Requires static libraries to be built." OFF)

  define_option(PARQUET_BUILD_EXAMPLES
                "Build the Parquet examples. Requires static libraries to be built." OFF)

  define_option(PARQUET_REQUIRE_ENCRYPTION
                "Build support for encryption. Fail if OpenSSL is not found" OFF)


  option(PEGASUS_BUILD_CONFIG_SUMMARY_JSON "Summarize build configuration in a JSON file"
         ON)
endif()

macro(config_summary_message)
  message(STATUS "---------------------------------------------------------------------")
  message(STATUS "Pegasus version:                                 ${PEGASUS_VERSION}")
  message(STATUS)
  message(STATUS "Build configuration summary:")

  message(STATUS "  Generator: ${CMAKE_GENERATOR}")
  message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
  message(STATUS "  Source directory: ${CMAKE_CURRENT_SOURCE_DIR}")
  message(STATUS "  Install prefix: ${CMAKE_INSTALL_PREFIX}")
  if(${CMAKE_EXPORT_COMPILE_COMMANDS})
    message(
      STATUS "  Compile commands: ${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json")
  endif()

  foreach(category ${PEGASUS_OPTION_CATEGORIES})

    message(STATUS)
    message(STATUS "${category} options:")

    set(option_names ${PEGASUS_${category}_OPTION_NAMES})

    set(max_value_length 0)
    foreach(name ${option_names})
      string(LENGTH "\"${${name}}\"" value_length)
      if(${max_value_length} LESS ${value_length})
        set(max_value_length ${value_length})
      endif()
    endforeach()

    foreach(name ${option_names})
      if("${${name}_OPTION_TYPE}" STREQUAL "string")
        set(value "\"${${name}}\"")
      else()
        set(value "${${name}}")
      endif()

      set(default ${${name}_OPTION_DEFAULT})
      set(description ${${name}_OPTION_DESCRIPTION})
      string(LENGTH ${description} description_length)
      if(${description_length} LESS 70)
        string(
          SUBSTRING
            "                                                                     "
            ${description_length} -1 description_padding)
      else()
        set(description_padding "
                                                                           ")
      endif()

      set(comment "[${name}]")

      if("${value}" STREQUAL "${default}")
        set(comment "[default] ${comment}")
      endif()

      if(NOT ("${${name}_OPTION_ENUM}" STREQUAL ""))
        set(comment "${comment} [${${name}_OPTION_ENUM}]")
      endif()

      string(
        SUBSTRING "${value}                                                             "
                  0 ${max_value_length} value)

      message(STATUS "  ${description} ${description_padding} ${value} ${comment}")
    endforeach()

  endforeach()

endmacro()

macro(config_summary_json)
  set(summary "${CMAKE_CURRENT_BINARY_DIR}/cmake_summary.json")
  message(STATUS "  Outputting build configuration summary to ${summary}")
  file(WRITE ${summary} "{\n")

  foreach(category ${PEGASUS_OPTION_CATEGORIES})
    foreach(name ${PEGASUS_${category}_OPTION_NAMES})
      file(APPEND ${summary} "\"${name}\": \"${${name}}\",\n")
    endforeach()
  endforeach()

  file(APPEND ${summary} "\"generator\": \"${CMAKE_GENERATOR}\",\n")
  file(APPEND ${summary} "\"build_type\": \"${CMAKE_BUILD_TYPE}\",\n")
  file(APPEND ${summary} "\"source_dir\": \"${CMAKE_CURRENT_SOURCE_DIR}\",\n")
  if(${CMAKE_EXPORT_COMPILE_COMMANDS})
    file(APPEND ${summary} "\"compile_commands\": "
                           "\"${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json\",\n")
  endif()
  file(APPEND ${summary} "\"install_prefix\": \"${CMAKE_INSTALL_PREFIX}\",\n")
  file(APPEND ${summary} "\"pegasus_version\": \"${PEGASUS_VERSION}\"\n")
  file(APPEND ${summary} "}\n")
endmacro()

macro(config_summary_cmake_setters path)
  file(WRITE ${path} "# Options used to build pegasus:")

  foreach(category ${PEGASUS_OPTION_CATEGORIES})
    file(APPEND ${path} "\n\n## ${category} options:")
    foreach(name ${PEGASUS_${category}_OPTION_NAMES})
      file(APPEND ${path} "\nset(${name} \"${${name}}\")")
    endforeach()
  endforeach()

endmacro()
