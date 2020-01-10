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

message (STATUS "RPC ${BUILD_OUTPUT_ROOT_DIRECTORY}")
# where to put generated libraries
set(LIBRARY_OUTPUT_PATH "${BUILD_OUTPUT_ROOT_DIRECTORY}/rpc")

# where to put generated binaries
set(EXECUTABLE_OUTPUT_PATH "${BUILD_OUTPUT_ROOT_DIRECTORY}/rpc")

add_custom_target(pegasus_rpc)

pegasus_install_all_headers("pegasus/rpc")

set(PEGASUS_RPC_STATIC_LINK_LIBS
    ${PEGASUS_PROTOBUF_LIBPROTOBUF}
    gRPC::grpc++
    gRPC::grpc
    gRPC::gpr
    c-ares::cares)

if(WIN32)
  list(APPEND PEGASUS_RPC_STATIC_LINK_LIBS Ws2_32.lib)
endif()

if(GRPC_HAS_ADDRESS_SORTING)
  list(APPEND PEGASUS_RPC_STATIC_LINK_LIBS gRPC::address_sorting)
endif()

# TODO(wesm): Protobuf shared vs static linking

set(RPC_PROTO_PATH "${PEGASUS_SOURCE_DIR}/../format")
set(RPC_PROTO ${PEGASUS_SOURCE_DIR}/../format/Flight.proto)

set(PEGASUS_GENERATED_PROTO_FILES "${CMAKE_CURRENT_BINARY_DIR}/Flight.pb.cc"
                                 "${CMAKE_CURRENT_BINARY_DIR}/Flight.pb.h"
                                 "${CMAKE_CURRENT_BINARY_DIR}/Flight.grpc.pb.cc"
                                 "${CMAKE_CURRENT_BINARY_DIR}/Flight.grpc.pb.h")

set(PROTO_DEPENDS ${RPC_PROTO} ${PEGASUS_PROTOBUF_LIBPROTOBUF} gRPC::grpc)

add_custom_command(OUTPUT ${PEGASUS_GENERATED_PROTO_FILES}
                   COMMAND ${PEGASUS_PROTOBUF_PROTOC} "-I${RPC_PROTO_PATH}"
                           "--cpp_out=${CMAKE_CURRENT_BINARY_DIR}" "${RPC_PROTO}"
                   DEPENDS ${PROTO_DEPENDS} ARGS
                   COMMAND ${PEGASUS_PROTOBUF_PROTOC}
                           "-I${RPC_PROTO_PATH}"
                           "--grpc_out=${CMAKE_CURRENT_BINARY_DIR}"
                           "--plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN}"
                           "${RPC_PROTO}")

set_source_files_properties(${PEGASUS_GENERATED_PROTO_FILES} PROPERTIES GENERATED TRUE)

add_custom_target(pegasus_grpc_gen ALL DEPENDS ${PEGASUS_GENERATED_PROTO_FILES})

# Note, we do not compile the generated Protobuf sources directly, instead
# compiling then via protocol_internal.cc which contains some gRPC template
# overrides to enable Flight-specific optimizations. See comments in
# protobuf-internal.cc
set(PEGASUS_RPC_SRCS
    client.cc
    internal.cc
    protocol_internal.cc
    serialization_internal.cc
    server.cc
    server_auth.cc
    types.cc)

add_pegasus_lib(pegasus_rpc
              CMAKE_PACKAGE_NAME
              PegasusRPC
              PKG_CONFIG_NAME
              pegasus-rpc
              OUTPUTS
              PEGASUS_RPC_LIBRARIES
              SOURCES
              ${PEGASUS_RPC_SRCS}
              DEPENDENCIES
              pegasus_grpc_gen
              SHARED_LINK_FLAGS
              ${PEGASUS_VERSION_SCRIPT_FLAGS} # Defined in cpp/pegasus/CMakeLists.txt
              SHARED_LINK_LIBS
              pegasus_shared
              ${PEGASUS_RPC_STATIC_LINK_LIBS}
              STATIC_LINK_LIBS
              pegasus_static
              ${PEGASUS_RPC_STATIC_LINK_LIBS})

foreach(LIB_TARGET ${PEGASUS_RPC_LIBRARIES})
  target_compile_definitions(${LIB_TARGET} PRIVATE PEGASUS_RPC_EXPORTING)
endforeach()