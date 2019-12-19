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

# Common path suffixes to be searched by find_library or find_path.
# Windows artifacts may be found under "<root>/Library", so
# search there as well.
set(LIB_PATH_SUFFIXES
    "${CMAKE_LIBRARY_ARCHITECTURE}"
    "lib/${CMAKE_LIBRARY_ARCHITECTURE}"
    "lib64"
    "lib32"
    "lib"
    "bin"
    "Library"
    "Library/lib"
    "Library/bin")
set(INCLUDE_PATH_SUFFIXES "include" "Library" "Library/include")

function(ADD_THIRDPARTY_LIB LIB_NAME)
  set(options)
  set(one_value_args SHARED_LIB STATIC_LIB)
  set(multi_value_args DEPS INCLUDE_DIRECTORIES)
  cmake_parse_arguments(ARG
                        "${options}"
                        "${one_value_args}"
                        "${multi_value_args}"
                        ${ARGN})
  if(ARG_UNPARSED_ARGUMENTS)
    message(SEND_ERROR "Error: unrecognized arguments: ${ARG_UNPARSED_ARGUMENTS}")
  endif()

  if(ARG_STATIC_LIB AND ARG_SHARED_LIB)
    set(AUG_LIB_NAME "${LIB_NAME}_static")
    add_library(${AUG_LIB_NAME} STATIC IMPORTED)
    set_target_properties(${AUG_LIB_NAME}
                          PROPERTIES IMPORTED_LOCATION "${ARG_STATIC_LIB}")
    if(ARG_DEPS)
      set_target_properties(${AUG_LIB_NAME}
                            PROPERTIES INTERFACE_LINK_LIBRARIES "${ARG_DEPS}")
    endif()
    message(STATUS "Added static library dependency ${AUG_LIB_NAME}: ${ARG_STATIC_LIB}")
    if(ARG_INCLUDE_DIRECTORIES)
      set_target_properties(${AUG_LIB_NAME}
                            PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
                                       "${ARG_INCLUDE_DIRECTORIES}")
    endif()

    set(AUG_LIB_NAME "${LIB_NAME}_shared")
    add_library(${AUG_LIB_NAME} SHARED IMPORTED)

    if(WIN32)
      # Mark the ".lib" location as part of a Windows DLL
      set_target_properties(${AUG_LIB_NAME}
                            PROPERTIES IMPORTED_IMPLIB "${ARG_SHARED_LIB}")
    else()
      set_target_properties(${AUG_LIB_NAME}
                            PROPERTIES IMPORTED_LOCATION "${ARG_SHARED_LIB}")
    endif()
    if(ARG_DEPS)
      set_target_properties(${AUG_LIB_NAME}
                            PROPERTIES INTERFACE_LINK_LIBRARIES "${ARG_DEPS}")
    endif()
    message(STATUS "Added shared library dependency ${AUG_LIB_NAME}: ${ARG_SHARED_LIB}")
    if(ARG_INCLUDE_DIRECTORIES)
      set_target_properties(${AUG_LIB_NAME}
                            PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
                                       "${ARG_INCLUDE_DIRECTORIES}")
    endif()
  elseif(ARG_STATIC_LIB)
    set(AUG_LIB_NAME "${LIB_NAME}_static")
    add_library(${AUG_LIB_NAME} STATIC IMPORTED)
    set_target_properties(${AUG_LIB_NAME}
                          PROPERTIES IMPORTED_LOCATION "${ARG_STATIC_LIB}")
    if(ARG_DEPS)
      set_target_properties(${AUG_LIB_NAME}
                            PROPERTIES INTERFACE_LINK_LIBRARIES "${ARG_DEPS}")
    endif()
    message(STATUS "Added static library dependency ${AUG_LIB_NAME}: ${ARG_STATIC_LIB}")
    if(ARG_INCLUDE_DIRECTORIES)
      set_target_properties(${AUG_LIB_NAME}
                            PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
                                       "${ARG_INCLUDE_DIRECTORIES}")
    endif()
  elseif(ARG_SHARED_LIB)
    set(AUG_LIB_NAME "${LIB_NAME}_shared")
    add_library(${AUG_LIB_NAME} SHARED IMPORTED)

    if(WIN32)
      # Mark the ".lib" location as part of a Windows DLL
      set_target_properties(${AUG_LIB_NAME}
                            PROPERTIES IMPORTED_IMPLIB "${ARG_SHARED_LIB}")
    else()
      set_target_properties(${AUG_LIB_NAME}
                            PROPERTIES IMPORTED_LOCATION "${ARG_SHARED_LIB}")
    endif()
    message(STATUS "Added shared library dependency ${AUG_LIB_NAME}: ${ARG_SHARED_LIB}")
    if(ARG_DEPS)
      set_target_properties(${AUG_LIB_NAME}
                            PROPERTIES INTERFACE_LINK_LIBRARIES "${ARG_DEPS}")
    endif()
    if(ARG_INCLUDE_DIRECTORIES)
      set_target_properties(${AUG_LIB_NAME}
                            PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
                                       "${ARG_INCLUDE_DIRECTORIES}")
    endif()
  else()
    message(FATAL_ERROR "No static or shared library provided for ${LIB_NAME}")
  endif()
endfunction()

# \arg OUTPUTS list to append built targets to
function(ADD_PEGASUS_LIB LIB_NAME)
  set(options BUILD_SHARED BUILD_STATIC)
  set(one_value_args CMAKE_PACKAGE_NAME PKG_CONFIG_NAME SHARED_LINK_FLAGS)
  set(multi_value_args
      SOURCES
      OUTPUTS
      STATIC_LINK_LIBS
      SHARED_LINK_LIBS
      SHARED_PRIVATE_LINK_LIBS
      EXTRA_INCLUDES
      PRIVATE_INCLUDES
      DEPENDENCIES
      SHARED_INSTALL_INTERFACE_LIBS
      STATIC_INSTALL_INTERFACE_LIBS
      OUTPUT_PATH)
  cmake_parse_arguments(ARG
                        "${options}"
                        "${one_value_args}"
                        "${multi_value_args}"
                        ${ARGN})
  if(ARG_UNPARSED_ARGUMENTS)
    message(SEND_ERROR "Error: unrecognized arguments: ${ARG_UNPARSED_ARGUMENTS}")
  endif()

  if(ARG_OUTPUTS)
    set(${ARG_OUTPUTS})
  endif()

  # Allow overriding PEGASUS_BUILD_SHARED and PEGASUS_BUILD_STATIC
  if(ARG_BUILD_SHARED)
    set(BUILD_SHARED ${ARG_BUILD_SHARED})
  else()
    set(BUILD_SHARED ${PEGASUS_BUILD_SHARED})
  endif()
  if(ARG_BUILD_STATIC)
    set(BUILD_STATIC ${ARG_BUILD_STATIC})
  else()
    set(BUILD_STATIC ${PEGASUS_BUILD_STATIC})
  endif()
  if(ARG_OUTPUT_PATH)
    set(OUTPUT_PATH ${ARG_OUTPUT_PATH})
  else()
    set(OUTPUT_PATH ${BUILD_OUTPUT_ROOT_DIRECTORY})
  endif()

  if(WIN32 OR (CMAKE_GENERATOR STREQUAL Xcode))
    # We need to compile C++ separately for each library kind (shared and static)
    # because of dllexport declarations on Windows.
    # The Xcode generator doesn't reliably work with Xcode as target names are not
    # guessed correctly.
    set(LIB_DEPS ${ARG_SOURCES})
    set(EXTRA_DEPS ${ARG_DEPENDENCIES})

    if(ARG_EXTRA_INCLUDES)
      set(LIB_INCLUDES ${ARG_EXTRA_INCLUDES})
    endif()
  else()
    # Otherwise, generate a single "objlib" from all C++ modules and link
    # that "objlib" into each library kind, to avoid compiling twice
    add_library(${LIB_NAME}_objlib OBJECT ${ARG_SOURCES})
    # Necessary to make static linking into other shared libraries work properly
    set_property(TARGET ${LIB_NAME}_objlib PROPERTY POSITION_INDEPENDENT_CODE 1)
    if(ARG_DEPENDENCIES)
      add_dependencies(${LIB_NAME}_objlib ${ARG_DEPENDENCIES})
    endif()
    set(LIB_DEPS $<TARGET_OBJECTS:${LIB_NAME}_objlib>)
    set(LIB_INCLUDES)
    set(EXTRA_DEPS)

    if(ARG_OUTPUTS)
      list(APPEND ${ARG_OUTPUTS} ${LIB_NAME}_objlib)
    endif()

    if(ARG_EXTRA_INCLUDES)
      target_include_directories(${LIB_NAME}_objlib SYSTEM PUBLIC ${ARG_EXTRA_INCLUDES})
    endif()
    if(ARG_PRIVATE_INCLUDES)
      target_include_directories(${LIB_NAME}_objlib PRIVATE ${ARG_PRIVATE_INCLUDES})
    endif()
  endif()

  set(RUNTIME_INSTALL_DIR bin)

  if(BUILD_SHARED)
    add_library(${LIB_NAME}_shared SHARED ${LIB_DEPS})
    if(EXTRA_DEPS)
      add_dependencies(${LIB_NAME}_shared ${EXTRA_DEPS})
    endif()

    if(ARG_OUTPUTS)
      list(APPEND ${ARG_OUTPUTS} ${LIB_NAME}_shared)
    endif()

    if(LIB_INCLUDES)
      target_include_directories(${LIB_NAME}_shared SYSTEM PUBLIC ${ARG_EXTRA_INCLUDES})
    endif()

    if(ARG_PRIVATE_INCLUDES)
      target_include_directories(${LIB_NAME}_shared PRIVATE ${ARG_PRIVATE_INCLUDES})
    endif()

    if(APPLE AND NOT DEFINED $ENV{EMSCRIPTEN})
      # On OS X, you can avoid linking at library load time and instead
      # expecting that the symbols have been loaded separately. This happens
      # with libpython* where there can be conflicts between system Python and
      # the Python from a thirdparty distribution
      #
      # When running with the Emscripten Compiler, we need not worry about
      # python, and the Emscripten Compiler does not support this option.
      set(ARG_SHARED_LINK_FLAGS "-undefined dynamic_lookup ${ARG_SHARED_LINK_FLAGS}")
    endif()

    set_target_properties(${LIB_NAME}_shared
                          PROPERTIES LIBRARY_OUTPUT_DIRECTORY
                                     "${OUTPUT_PATH}"
                                     RUNTIME_OUTPUT_DIRECTORY
                                     "${OUTPUT_PATH}"
                                     PDB_OUTPUT_DIRECTORY
                                     "${OUTPUT_PATH}"
                                     LINK_FLAGS
                                     "${ARG_SHARED_LINK_FLAGS}"
                                     OUTPUT_NAME
                                     ${LIB_NAME})

    target_link_libraries(${LIB_NAME}_shared
                          LINK_PUBLIC
                          "$<BUILD_INTERFACE:${ARG_SHARED_LINK_LIBS}>"
                          "$<INSTALL_INTERFACE:${ARG_SHARED_INSTALL_INTERFACE_LIBS}>"
                          LINK_PRIVATE
                          ${ARG_SHARED_PRIVATE_LINK_LIBS})

    install(TARGETS ${LIB_NAME}_shared ${INSTALL_IS_OPTIONAL}
            EXPORT ${LIB_NAME}_targets
            RUNTIME DESTINATION ${RUNTIME_INSTALL_DIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
  endif()

  if(BUILD_STATIC)
    add_library(${LIB_NAME}_static STATIC ${LIB_DEPS})
    if(EXTRA_DEPS)
      add_dependencies(${LIB_NAME}_static ${EXTRA_DEPS})
    endif()

    if(ARG_OUTPUTS)
      list(APPEND ${ARG_OUTPUTS} ${LIB_NAME}_static)
    endif()

    if(LIB_INCLUDES)
      target_include_directories(${LIB_NAME}_static SYSTEM PUBLIC ${ARG_EXTRA_INCLUDES})
    endif()

    if(ARG_PRIVATE_INCLUDES)
      target_include_directories(${LIB_NAME}_static PRIVATE ${ARG_PRIVATE_INCLUDES})
    endif()

    if(MSVC)
      set(LIB_NAME_STATIC ${LIB_NAME}_static)
    else()
      set(LIB_NAME_STATIC ${LIB_NAME})
    endif()

    set_target_properties(${LIB_NAME}_static
                          PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_PATH}" OUTPUT_NAME
                                     ${LIB_NAME_STATIC})

    if(ARG_STATIC_INSTALL_INTERFACE_LIBS)
      set(INTERFACE_LIBS ${ARG_STATIC_INSTALL_INTERFACE_LIBS})
    else()
      set(INTERFACE_LIBS ${ARG_STATIC_LINK_LIBS})
    endif()

    target_link_libraries(${LIB_NAME}_static LINK_PUBLIC
                          "$<BUILD_INTERFACE:${ARG_STATIC_LINK_LIBS}>"
                          "$<INSTALL_INTERFACE:${INTERFACE_LIBS}>")

    install(TARGETS ${LIB_NAME}_static ${INSTALL_IS_OPTIONAL}
            EXPORT ${LIB_NAME}_targets
            RUNTIME DESTINATION ${RUNTIME_INSTALL_DIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
  endif()

  if(ARG_CMAKE_PACKAGE_NAME)
    pegasus_install_cmake_find_module("${ARG_CMAKE_PACKAGE_NAME}")

    set(TARGETS_CMAKE "${ARG_CMAKE_PACKAGE_NAME}Targets.cmake")
    install(EXPORT ${LIB_NAME}_targets
            FILE "${TARGETS_CMAKE}"
            DESTINATION "${PEGASUS_CMAKE_INSTALL_DIR}")

    set(CONFIG_CMAKE "${ARG_CMAKE_PACKAGE_NAME}Config.cmake")
    set(BUILT_CONFIG_CMAKE "${CMAKE_CURRENT_BINARY_DIR}/${CONFIG_CMAKE}")
    configure_package_config_file("${CONFIG_CMAKE}.in" "${BUILT_CONFIG_CMAKE}"
                                  INSTALL_DESTINATION "${PEGASUS_CMAKE_INSTALL_DIR}")
    install(FILES "${BUILT_CONFIG_CMAKE}" DESTINATION "${PEGASUS_CMAKE_INSTALL_DIR}")

    set(CONFIG_VERSION_CMAKE "${ARG_CMAKE_PACKAGE_NAME}ConfigVersion.cmake")
    set(BUILT_CONFIG_VERSION_CMAKE "${CMAKE_CURRENT_BINARY_DIR}/${CONFIG_VERSION_CMAKE}")
    write_basic_package_version_file("${BUILT_CONFIG_VERSION_CMAKE}"
                                     VERSION ${${PROJECT_NAME}_VERSION}
                                     COMPATIBILITY AnyNewerVersion)
    install(FILES "${BUILT_CONFIG_VERSION_CMAKE}"
            DESTINATION "${PEGASUS_CMAKE_INSTALL_DIR}")
  endif()

  if(ARG_PKG_CONFIG_NAME)
    pegasus_add_pkg_config("${ARG_PKG_CONFIG_NAME}")
  endif()

  # Modify variable in calling scope
  if(ARG_OUTPUTS)
    set(${ARG_OUTPUTS} ${${ARG_OUTPUTS}} PARENT_SCOPE)
  endif()
endfunction()

function(PEGASUS_INSTALL_ALL_HEADERS PATH)
  set(options)
  set(one_value_args)
  set(multi_value_args PATTERN)
  cmake_parse_arguments(ARG
                        "${options}"
                        "${one_value_args}"
                        "${multi_value_args}"
                        ${ARGN})
  if(NOT ARG_PATTERN)
    # The .hpp extension is used by some vendored libraries
    set(ARG_PATTERN "*.h" "*.hpp")
  endif()
  file(GLOB CURRENT_DIRECTORY_HEADERS ${ARG_PATTERN})

  set(PUBLIC_HEADERS)
  foreach(HEADER ${CURRENT_DIRECTORY_HEADERS})
    get_filename_component(HEADER_BASENAME ${HEADER} NAME)
    if(HEADER_BASENAME MATCHES "internal")
      continue()
    endif()
    list(APPEND PUBLIC_HEADERS ${HEADER})
  endforeach()
  install(FILES ${PUBLIC_HEADERS} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/${PATH}")
endfunction()

function(PEGASUS_ADD_PKG_CONFIG MODULE)
  configure_file(${MODULE}.pc.in "${CMAKE_CURRENT_BINARY_DIR}/${MODULE}.pc" @ONLY)
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${MODULE}.pc"
          DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig/")
endfunction()

function(PEGASUS_INSTALL_CMAKE_FIND_MODULE MODULE)
  install(FILES "$PEGASUS_SOURCE_DIR}/cmake_modules/Find${MODULE}.cmake"
          DESTINATION "${PEGASUS_CMAKE_INSTALL_DIR}")
endfunction()
