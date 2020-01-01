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

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(arrow
                                  REQUIRED_VARS
                                  ARROW_INCLUDE_DIR
                                  VERSION_VAR
                                  ARROW_VERSION)


set(ARROW_SEARCH_LIB_PATH_SUFFIXES)
if(CMAKE_LIBRARY_ARCHITECTURE)
  list(APPEND ARROW_SEARCH_LIB_PATH_SUFFIXES "lib/${CMAKE_LIBRARY_ARCHITECTURE}")
endif()
list(APPEND ARROW_SEARCH_LIB_PATH_SUFFIXES
            "lib64"
            "lib32"
            "lib"
            "bin")
set(ARROW_CONFIG_SUFFIXES
    "_RELEASE"
    "_RELWITHDEBINFO"
    "_MINSIZEREL"
    "_DEBUG"
    "")
                                  
# Internal macro only for arrow_find_package.
#
# Find package by pkg-config.
macro(arrow_find_package_pkg_config)
  pkg_check_modules(${prefix}_PC ${pkg_config_name})
  if(${prefix}_PC_FOUND)
    set(${prefix}_USE_PKG_CONFIG TRUE PARENT_SCOPE)

    set(include_dir "${${prefix}_PC_INCLUDEDIR}")
    set(lib_dir "${${prefix}_PC_LIBDIR}")
    set(shared_lib_paths "${${prefix}_PC_LINK_LIBRARIES}")
    # Use the first shared library path as the IMPORTED_LOCATION
    # for ${target_shared}. This assumes that the first shared library
    # path is the shared library path for this module.
    list(GET shared_lib_paths 0 first_shared_lib_path)
    # Use the rest shared library paths as the INTERFACE_LINK_LIBRARIES
    # for ${target_shared}. This assumes that the rest shared library
    # paths are dependency library paths for this module.
    list(LENGTH shared_lib_paths n_shared_lib_paths)
    if(n_shared_lib_paths LESS_EQUAL 1)
      set(rest_shared_lib_paths)
    else()
      list(SUBLIST
           shared_lib_paths
           1
           -1
           rest_shared_lib_paths)
    endif()

    set(${prefix}_VERSION "${${prefix}_PC_VERSION}" PARENT_SCOPE)
    set(${prefix}_INCLUDE_DIR "${include_dir}" PARENT_SCOPE)
    set(${prefix}_SHARED_LIB "${first_shared_lib_path}" PARENT_SCOPE)

    add_library(${target_shared} SHARED IMPORTED)
    set_target_properties(${target_shared}
                          PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
                                     "${include_dir}"
                                     INTERFACE_LINK_LIBRARIES
                                     "${rest_shared_lib_paths}"
                                     IMPORTED_LOCATION
                                     "${first_shared_lib_path}")
    get_target_property(shared_lib ${target_shared} IMPORTED_LOCATION)

    find_library(${prefix}_static_lib
                 NAMES "${static_lib_name}"
                 PATHS "${lib_dir}"
                 NO_DEFAULT_PATH)
    set(static_lib "${${prefix}_static_lib}")
    set(${prefix}_STATIC_LIB "${static_lib}" PARENT_SCOPE)
    if(static_lib)
      add_library(${target_static} STATIC IMPORTED)
      set_target_properties(${target_static}
                            PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${include_dir}"
                                       IMPORTED_LOCATION "${static_lib}")
    endif()
  endif()
endmacro()

function(arrow_build_shared_library_name output_variable base_name)
  set(${output_variable}
      "${CMAKE_SHARED_LIBRARY_PREFIX}${base_name}${CMAKE_SHARED_LIBRARY_SUFFIX}"
      PARENT_SCOPE)
endfunction()

function(arrow_build_import_library_name output_variable base_name)
  set(${output_variable}
      "${CMAKE_IMPORT_LIBRARY_PREFIX}${base_name}${CMAKE_IMPORT_LIBRARY_SUFFIX}"
      PARENT_SCOPE)
endfunction()

function(arrow_build_static_library_name output_variable base_name)
  set(
    ${output_variable}
    "${CMAKE_STATIC_LIBRARY_PREFIX}${base_name}${ARROW_MSVC_STATIC_LIB_SUFFIX}${CMAKE_STATIC_LIBRARY_SUFFIX}"
    PARENT_SCOPE)
endfunction()

macro(arrow_find_package_home)
  find_path(${prefix}_include_dir "${header_path}"
            PATHS "${home}"
            PATH_SUFFIXES "include"
            NO_DEFAULT_PATH)
  set(include_dir "${${prefix}_include_dir}")
  set(${prefix}_INCLUDE_DIR "${include_dir}" PARENT_SCOPE)

  if(MSVC)
    set(CMAKE_SHARED_LIBRARY_SUFFIXES_ORIGINAL ${CMAKE_FIND_LIBRARY_SUFFIXES})
    # .dll isn't found by find_library with MSVC because .dll isn't included in
    # CMAKE_FIND_LIBRARY_SUFFIXES.
    list(APPEND CMAKE_FIND_LIBRARY_SUFFIXES "${CMAKE_SHARED_LIBRARY_SUFFIX}")
  endif()
  find_library(${prefix}_shared_lib
               NAMES "${shared_lib_name}"
               PATHS "${home}"
               PATH_SUFFIXES ${ARROW_SEARCH_LIB_PATH_SUFFIXES}
               NO_DEFAULT_PATH)
  if(MSVC)
    set(CMAKE_SHARED_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES_ORIGINAL})
  endif()
  set(shared_lib "${${prefix}_shared_lib}")
  set(${prefix}_SHARED_LIB "${shared_lib}" PARENT_SCOPE)
  if(shared_lib)
    add_library(${target_shared} SHARED IMPORTED)
    set_target_properties(${target_shared} PROPERTIES IMPORTED_LOCATION "${shared_lib}")
    if(include_dir)
      set_target_properties(${target_shared}
                            PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${include_dir}")
    endif()
    find_library(${prefix}_import_lib
                 NAMES "${import_lib_name}"
                 PATHS "${home}"
                 PATH_SUFFIXES ${ARROW_SEARCH_LIB_PATH_SUFFIXES}
                 NO_DEFAULT_PATH)
    set(import_lib "${${prefix}_import_lib}")
    set(${prefix}_IMPORT_LIB "${import_lib}" PARENT_SCOPE)
    if(import_lib)
      set_target_properties(${target_shared} PROPERTIES IMPORTED_IMPLIB "${import_lib}")
    endif()
  endif()

  find_library(${prefix}_static_lib
               NAMES "${static_lib_name}"
               PATHS "${home}"
               PATH_SUFFIXES ${ARROW_SEARCH_LIB_PATH_SUFFIXES}
               NO_DEFAULT_PATH)
  set(static_lib "${${prefix}_static_lib}")
  set(${prefix}_STATIC_LIB "${static_lib}" PARENT_SCOPE)
  if(static_lib)
    add_library(${target_static} STATIC IMPORTED)
    set_target_properties(${target_static} PROPERTIES IMPORTED_LOCATION "${static_lib}")
    if(include_dir)
      set_target_properties(${target_static}
                            PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${include_dir}")
    endif()
  endif()
endmacro()

function(arrow_find_package
         prefix
         home
         base_name
         header_path
         cmake_package_name
         pkg_config_name)
  arrow_build_shared_library_name(shared_lib_name ${base_name})
  arrow_build_import_library_name(import_lib_name ${base_name})
  arrow_build_static_library_name(static_lib_name ${base_name})

  set(target_shared ${base_name}_shared)
  set(target_static ${base_name}_static)

  if(home)
    arrow_find_package_home()
    set(${prefix}_FIND_APPROACH "HOME: ${home}" PARENT_SCOPE)
  else()
    arrow_find_package_cmake_package_configuration()
    if(${cmake_package_name}_FOUND)
      set(${prefix}_FIND_APPROACH
          "CMake package configuration: ${cmake_package_name}"
          PARENT_SCOPE)
    else()
      arrow_find_package_pkg_config()
      set(${prefix}_FIND_APPROACH "pkg-config: ${pkg_config_name}" PARENT_SCOPE)
    endif()
  endif()

  if(NOT include_dir)
    if(TARGET ${target_shared})
      get_target_property(include_dir ${target_shared} INTERFACE_INCLUDE_DIRECTORIES)
    elseif(TARGET ${target_static})
      get_target_property(include_dir ${target_static} INTERFACE_INCLUDE_DIRECTORIES)
    endif()
  endif()
  if(include_dir)
    set(${prefix}_INCLUDE_DIR "${include_dir}" PARENT_SCOPE)
  endif()

  if(shared_lib)
    get_filename_component(lib_dir "${shared_lib}" DIRECTORY)
  elseif(static_lib)
    get_filename_component(lib_dir "${static_lib}" DIRECTORY)
  else()
    set(lib_dir NOTFOUND)
  endif()
  set(${prefix}_LIB_DIR "${lib_dir}" PARENT_SCOPE)
  # For backward compatibility
  set(${prefix}_LIBS "${lib_dir}" PARENT_SCOPE)
endfunction()

set(find_package_arguments)
if(${CMAKE_FIND_PACKAGE_NAME}_FIND_VERSION)
  list(APPEND find_package_arguments "${${CMAKE_FIND_PACKAGE_NAME}_FIND_VERSION}")
endif()
if(${CMAKE_FIND_PACKAGE_NAME}_FIND_REQUIRED)
  list(APPEND find_package_arguments REQUIRED)
endif()
if(${CMAKE_FIND_PACKAGE_NAME}_FIND_QUIETLY)
  list(APPEND find_package_arguments QUIET)
endif()
find_package(Arrow ${find_package_arguments})
