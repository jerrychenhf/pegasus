// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#include "rpc/internal.h"
#include "rpc/platform.h"
#include "rpc/protocol_internal.h"

#include <cstddef>
#include <memory>
#include <sstream>
#include <string>
#include <utility>

#ifdef GRPCPP_PP_INCLUDE
#include <grpcpp/grpcpp.h>
#else
#include <grpc++/grpc++.h>
#endif

#include "arrow/buffer.h"
#include "arrow/io/memory.h"
#include "arrow/ipc/reader.h"
#include "arrow/ipc/writer.h"
#include "arrow/memory_pool.h"
#include "arrow/status.h"
#include "arrow/util/logging.h"
#include "arrow/util/string_builder.h"

namespace pegasus {
namespace rpc {
namespace internal {

const char* kGrpcAuthHeader = "auth-token-bin";

arrow::Status FromGrpcStatus(const grpc::Status& grpc_status) {
  if (grpc_status.ok()) {
    return arrow::Status::OK();
  }

  switch (grpc_status.error_code()) {
    case grpc::StatusCode::OK:
      return arrow::Status::OK();
    case grpc::StatusCode::CANCELLED:
      return arrow::Status::IOError("gRPC cancelled call, with message: ",
                             grpc_status.error_message())
          .WithDetail(std::make_shared<FlightStatusDetail>(FlightStatusCode::Cancelled));
    case grpc::StatusCode::UNKNOWN: {
      std::stringstream ss;
      ss << "Flight RPC failed with message: " << grpc_status.error_message();
      return arrow::Status::UnknownError(ss.str()).WithDetail(
          std::make_shared<FlightStatusDetail>(FlightStatusCode::Failed));
    }
    case grpc::StatusCode::INVALID_ARGUMENT:
      return arrow::Status::Invalid("gRPC returned invalid argument error, with message: ",
                             grpc_status.error_message());
    case grpc::StatusCode::DEADLINE_EXCEEDED:
      return arrow::Status::IOError("gRPC returned deadline exceeded error, with message: ",
                             grpc_status.error_message())
          .WithDetail(std::make_shared<FlightStatusDetail>(FlightStatusCode::TimedOut));
    case grpc::StatusCode::NOT_FOUND:
      return arrow::Status::KeyError("gRPC returned not found error, with message: ",
                              grpc_status.error_message());
    case grpc::StatusCode::ALREADY_EXISTS:
      return arrow::Status::AlreadyExists("gRPC returned already exists error, with message: ",
                                   grpc_status.error_message());
    case grpc::StatusCode::PERMISSION_DENIED:
      return arrow::Status::IOError("gRPC returned permission denied error, with message: ",
                             grpc_status.error_message())
          .WithDetail(
              std::make_shared<FlightStatusDetail>(FlightStatusCode::Unauthorized));
    case grpc::StatusCode::RESOURCE_EXHAUSTED:
      return arrow::Status::Invalid("gRPC returned resource exhausted error, with message: ",
                             grpc_status.error_message());
    case grpc::StatusCode::FAILED_PRECONDITION:
      return arrow::Status::Invalid("gRPC returned precondition failed error, with message: ",
                             grpc_status.error_message());
    case grpc::StatusCode::ABORTED:
      return arrow::Status::IOError("gRPC returned aborted error, with message: ",
                             grpc_status.error_message())
          .WithDetail(std::make_shared<FlightStatusDetail>(FlightStatusCode::Internal));
    case grpc::StatusCode::OUT_OF_RANGE:
      return arrow::Status::Invalid("gRPC returned out-of-range error, with message: ",
                             grpc_status.error_message());
    case grpc::StatusCode::UNIMPLEMENTED:
      return arrow::Status::NotImplemented("gRPC returned unimplemented error, with message: ",
                                    grpc_status.error_message());
    case grpc::StatusCode::INTERNAL:
      return arrow::Status::IOError("gRPC returned internal error, with message: ",
                             grpc_status.error_message())
          .WithDetail(std::make_shared<FlightStatusDetail>(FlightStatusCode::Internal));
    case grpc::StatusCode::UNAVAILABLE:
      return arrow::Status::IOError("gRPC returned unavailable error, with message: ",
                             grpc_status.error_message())
          .WithDetail(
              std::make_shared<FlightStatusDetail>(FlightStatusCode::Unavailable));
    case grpc::StatusCode::DATA_LOSS:
      return arrow::Status::IOError("gRPC returned data loss error, with message: ",
                             grpc_status.error_message())
          .WithDetail(std::make_shared<FlightStatusDetail>(FlightStatusCode::Internal));
    case grpc::StatusCode::UNAUTHENTICATED:
      return arrow::Status::IOError("gRPC returned unauthenticated error, with message: ",
                             grpc_status.error_message())
          .WithDetail(
              std::make_shared<FlightStatusDetail>(FlightStatusCode::Unauthenticated));
    default:
      return arrow::Status::UnknownError("gRPC failed with error code ",
                                  grpc_status.error_code(),
                                  " and message: ", grpc_status.error_message());
  }
}

grpc::Status ToGrpcStatus(const arrow::Status& arrow_status) {
  if (arrow_status.ok()) {
    return grpc::Status::OK;
  }

  grpc::StatusCode grpc_code = grpc::StatusCode::UNKNOWN;
  std::string message = arrow_status.message();
  if (arrow_status.detail()) {
    message += ". Detail: ";
    message += arrow_status.detail()->ToString();
  }

  std::shared_ptr<FlightStatusDetail> flight_status =
      FlightStatusDetail::UnwrapStatus(arrow_status);
  if (flight_status) {
    switch (flight_status->code()) {
      case FlightStatusCode::Internal:
        grpc_code = grpc::StatusCode::INTERNAL;
        break;
      case FlightStatusCode::TimedOut:
        grpc_code = grpc::StatusCode::DEADLINE_EXCEEDED;
        break;
      case FlightStatusCode::Cancelled:
        grpc_code = grpc::StatusCode::CANCELLED;
        break;
      case FlightStatusCode::Unauthenticated:
        grpc_code = grpc::StatusCode::UNAUTHENTICATED;
        break;
      case FlightStatusCode::Unauthorized:
        grpc_code = grpc::StatusCode::PERMISSION_DENIED;
        break;
      case FlightStatusCode::Unavailable:
        grpc_code = grpc::StatusCode::UNAVAILABLE;
        break;
      default:
        break;
    }
  } else if (arrow_status.IsNotImplemented()) {
    grpc_code = grpc::StatusCode::UNIMPLEMENTED;
  } else if (arrow_status.IsInvalid()) {
    grpc_code = grpc::StatusCode::INVALID_ARGUMENT;
  }
  return grpc::Status(grpc_code, message);
}

// ActionType

arrow::Status FromProto(const pb::ActionType& pb_type, ActionType* type) {
  type->type = pb_type.type();
  type->description = pb_type.description();
  return arrow::Status::OK();
}

arrow::Status ToProto(const ActionType& type, pb::ActionType* pb_type) {
  pb_type->set_type(type.type);
  pb_type->set_description(type.description);
  return arrow::Status::OK();
}

// Action

arrow::Status FromProto(const pb::Action& pb_action, Action* action) {
  action->type = pb_action.type();
  return arrow::Buffer::FromString(pb_action.body(), &action->body);
}

arrow::Status ToProto(const Action& action, pb::Action* pb_action) {
  pb_action->set_type(action.type);
  if (action.body) {
    pb_action->set_body(action.body->ToString());
  }
  return arrow::Status::OK();
}

// Result (of an Action)

arrow::Status FromProto(const pb::Result& pb_result, Result* result) {
  // ARROW-3250; can avoid copy. Can also write custom deserializer if it
  // becomes an issue
  return arrow::Buffer::FromString(pb_result.body(), &result->body);
}

arrow::Status ToProto(const Result& result, pb::Result* pb_result) {
  pb_result->set_body(result.body->ToString());
  return arrow::Status::OK();
}

// Criteria

arrow::Status FromProto(const pb::Criteria& pb_criteria, Criteria* criteria) {
  return arrow::Status::OK();
}

// Location

arrow::Status FromProto(const pb::Location& pb_location, Location* location) {
  return Location::Parse(pb_location.uri(), location);
}

void ToProto(const Location& location, pb::Location* pb_location) {
  pb_location->set_uri(location.ToString());
}

arrow::Status ToProto(const BasicAuth& basic_auth, pb::BasicAuth* pb_basic_auth) {
  pb_basic_auth->set_username(basic_auth.username);
  pb_basic_auth->set_password(basic_auth.password);
  return arrow::Status::OK();
}

// Ticket

arrow::Status FromProto(const pb::Ticket& pb_ticket, Ticket* ticket) {
  ticket->dataset_path = pb_ticket.dataset_path();
  ticket->partition_identity = pb_ticket.partition_identity();
  ticket->schema = pb_ticket.schema();
  ticket->column_indices.reserve(pb_ticket.column_indice_size());
  for (int i = 0; i < pb_ticket.column_indice_size(); ++i) {
    ticket->column_indices.emplace_back(pb_ticket.column_indice(i));
  }
  return arrow::Status::OK();
}

void ToProto(const Ticket& ticket, pb::Ticket* pb_ticket) {
  pb_ticket->set_dataset_path(ticket.dataset_path);
  pb_ticket->set_partition_identity(ticket.partition_identity);
  pb_ticket->set_schema(ticket.serialized_schema());
  for (int32_t column_indice : ticket.column_indices) {
    pb_ticket->add_column_indice(column_indice);
  }
}

// FlightData

arrow::Status FromProto(const pb::FlightData& pb_data, FlightDescriptor* descriptor,
                 std::unique_ptr<arrow::ipc::Message>* message) {
  RETURN_NOT_OK(internal::FromProto(pb_data.flight_descriptor(), descriptor));
  const std::string& header = pb_data.data_header();
  const std::string& body = pb_data.data_body();
  std::shared_ptr<arrow::Buffer> header_buf = arrow::Buffer::Wrap(header.data(), header.size());
  std::shared_ptr<arrow::Buffer> body_buf = arrow::Buffer::Wrap(body.data(), body.size());
  if (header_buf == nullptr || body_buf == nullptr) {
    return arrow::Status::UnknownError("Could not create buffers from protobuf");
  }
  return arrow::ipc::Message::Open(header_buf, body_buf, message);
}

// FlightEndpoint

arrow::Status FromProto(const pb::FlightEndpoint& pb_endpoint, FlightEndpoint* endpoint) {
  RETURN_NOT_OK(FromProto(pb_endpoint.ticket(), &endpoint->ticket));
  endpoint->locations.resize(pb_endpoint.location_size());
  for (int i = 0; i < pb_endpoint.location_size(); ++i) {
    RETURN_NOT_OK(FromProto(pb_endpoint.location(i), &endpoint->locations[i]));
  }
  return arrow::Status::OK();
}

void ToProto(const FlightEndpoint& endpoint, pb::FlightEndpoint* pb_endpoint) {
  ToProto(endpoint.ticket, pb_endpoint->mutable_ticket());
  pb_endpoint->clear_location();
  for (const Location& location : endpoint.locations) {
    ToProto(location, pb_endpoint->add_location());
  }
}

// FlightDescriptor

arrow::Status FromProto(const pb::FlightDescriptor& pb_descriptor,
                 FlightDescriptor* descriptor) {
  if (pb_descriptor.type() == pb::FlightDescriptor::PATH) {
    descriptor->type = FlightDescriptor::PATH;
    descriptor->path.reserve(pb_descriptor.path_size());
    for (int i = 0; i < pb_descriptor.path_size(); ++i) {
      descriptor->path.emplace_back(pb_descriptor.path(i));
    }
  } else if (pb_descriptor.type() == pb::FlightDescriptor::CMD) {
    descriptor->type = FlightDescriptor::CMD;
    descriptor->cmd = pb_descriptor.cmd();
  } else {
    return arrow::Status::Invalid("Client sent UNKNOWN descriptor type");
  }
  
  descriptor->properties.clear();
  const google::protobuf::Map<std::string, std::string>& properties =
    pb_descriptor.properties();
  for(google::protobuf::Map<std::string, std::string>::const_iterator it = properties.begin();
      it != properties.end(); ++it) {
      descriptor->properties[it->first] = it->second;
  }
  return arrow::Status::OK();
}

arrow::Status ToProto(const FlightDescriptor& descriptor, pb::FlightDescriptor* pb_descriptor) {
  if (descriptor.type == FlightDescriptor::PATH) {
    pb_descriptor->set_type(pb::FlightDescriptor::PATH);
    for (const std::string& path : descriptor.path) {
      pb_descriptor->add_path(path);
    }
  } else {
    pb_descriptor->set_type(pb::FlightDescriptor::CMD);
    pb_descriptor->set_cmd(descriptor.cmd);
  }
  
  pb_descriptor->clear_properties();
  google::protobuf::Map<std::string, std::string>* properties =
    pb_descriptor->mutable_properties();
  for(std::unordered_map<std::string, std::string>::const_iterator it = descriptor.properties.begin();
      it != descriptor.properties.end(); ++it) {
      (*properties)[it->first] = it->second;
  }
  
  return arrow::Status::OK();
}

// FlightInfo

arrow::Status FromProto(const pb::FlightInfo& pb_info, FlightInfo::Data* info) {
  RETURN_NOT_OK(FromProto(pb_info.flight_descriptor(), &info->descriptor));

  info->schema = pb_info.schema();

  info->endpoints.resize(pb_info.endpoint_size());
  for (int i = 0; i < pb_info.endpoint_size(); ++i) {
    RETURN_NOT_OK(FromProto(pb_info.endpoint(i), &info->endpoints[i]));
  }

  info->total_records = pb_info.total_records();
  info->total_bytes = pb_info.total_bytes();
  return arrow::Status::OK();
}

arrow::Status FromProto(const pb::BasicAuth& pb_basic_auth, BasicAuth* basic_auth) {
  basic_auth->password = pb_basic_auth.password();
  basic_auth->username = pb_basic_auth.username();

  return arrow::Status::OK();
}

arrow::Status FromProto(const pb::SchemaResult& pb_result, std::string* result) {
  *result = pb_result.schema();
  return arrow::Status::OK();
}

arrow::Status SchemaToString(const arrow::Schema& schema, std::string* out) {
  // TODO(wesm): Do we care about better memory efficiency here?
  std::shared_ptr<arrow::Buffer> serialized_schema;
  arrow::ipc::DictionaryMemo unused_dict_memo;
  RETURN_NOT_OK(arrow::ipc::SerializeSchema(schema, &unused_dict_memo, arrow::default_memory_pool(),
                                     &serialized_schema));
  *out = std::string(reinterpret_cast<const char*>(serialized_schema->data()),
                     static_cast<size_t>(serialized_schema->size()));
  return arrow::Status::OK();
}

arrow::Status ToProto(const FlightInfo& info, pb::FlightInfo* pb_info) {
  // clear any repeated fields
  pb_info->clear_endpoint();

  pb_info->set_schema(info.serialized_schema());

  // descriptor
  RETURN_NOT_OK(ToProto(info.descriptor(), pb_info->mutable_flight_descriptor()));

  // endpoints
  for (const FlightEndpoint& endpoint : info.endpoints()) {
    ToProto(endpoint, pb_info->add_endpoint());
  }

  pb_info->set_total_records(info.total_records());
  pb_info->set_total_bytes(info.total_bytes());
  return arrow::Status::OK();
}

arrow::Status ToProto(const SchemaResult& result, pb::SchemaResult* pb_result) {
  pb_result->set_schema(result.serialized_schema());
  return arrow::Status::OK();
}

//NodeInfo

arrow::Status FromProto(const pb::NodeInfo& pb_info, NodeInfo* info) {
  info->cache_capacity = pb_info.cache_capacity();
  info->cache_free = pb_info.cache_free();
  info->total_cacherd_cnt = pb_info.total_cacherd_cnt();
  info->ds_cacherd_cnt = pb_info.ds_cacherd_cnt();
  info->pt_cacherd_cnt = pb_info.pt_cacherd_cnt();
  info->col_cacherd_cnt = pb_info.col_cacherd_cnt();
  return arrow::Status::OK();
}

arrow::Status ToProto(const NodeInfo& info, pb::NodeInfo* pb_info) {
  pb_info->set_cache_capacity(info.cache_capacity);
  pb_info->set_cache_free(info.cache_free);
  pb_info->set_total_cacherd_cnt(info.total_cacherd_cnt);
  pb_info->set_ds_cacherd_cnt(info.ds_cacherd_cnt);
  pb_info->set_pt_cacherd_cnt(info.pt_cacherd_cnt);
  pb_info->set_col_cacherd_cnt(info.col_cacherd_cnt);
  return arrow::Status::OK();
}

// HeartbeatInfo
arrow::Status FromProto(const pb::HeartbeatInfo& pb_info, HeartbeatInfo* info) {
  info->hostname = pb_info.hostname();
  info->port = pb_info.port();
  if (pb_info.type() == pb::HeartbeatInfo::REGISTRATION) {
    info->type = HeartbeatInfo::REGISTRATION;
    
    if (pb_info.has_address()) {
      FromProto(pb_info.address(), info->mutable_address());
    } else {
      return arrow::Status::Invalid("Address is required for registration.");
    }
  } else if (pb_info.type() == pb::HeartbeatInfo::HEARTBEAT) {
    info->type = HeartbeatInfo::HEARTBEAT;
  } else {
    return arrow::Status::Invalid("Client sent UNKNOWN heartbeat type");
  }
  
  // node info
  if (pb_info.has_node_info()) {
    FromProto(pb_info.node_info(), info->mutable_node_info());
  } else {
    info->node_info = nullptr;
  }
  return arrow::Status::OK();
}

arrow::Status ToProto(const HeartbeatInfo& info, pb::HeartbeatInfo* pb_info) {
  pb_info->set_hostname(info.hostname);
  pb_info->set_port(info.port);
  if (info.type == HeartbeatInfo::REGISTRATION) {
    pb_info->set_type(pb::HeartbeatInfo::REGISTRATION);
    
    // Address must be specified
    if (info.address != nullptr) {
      ToProto(*(info.address.get()), pb_info->mutable_address());
    } else {
      return arrow::Status::Invalid("Address must be specified for registration.");
    }
  } else if (info.type == HeartbeatInfo::HEARTBEAT) {
    pb_info->set_type(pb::HeartbeatInfo::HEARTBEAT);
  } else {
    return arrow::Status::Invalid("UNKNOWN heartbeat type");
  }
  
  // node info
  if (info.node_info != nullptr) {
    ToProto(*(info.node_info.get()), pb_info->mutable_node_info());
  } else {
    pb_info->clear_node_info();
  }
  return arrow::Status::OK();
}

// PartitionDropList
arrow::Status FromProto(const pb::PartitionDropList& pb_partsdroplist, PartitionDropList* partsdroplist) {
  partsdroplist->datasetpath = pb_partsdroplist.datasetpath();
  for (int i=0; i<pb_partsdroplist.partitions_size(); i++) {
    partsdroplist->partitions.emplace_back(pb_partsdroplist.partitions(i));
  }
  return arrow::Status::OK();
}

arrow::Status ToProto(const PartitionDropList& partsdroplist, pb::PartitionDropList* pb_partsdroplist) {
  pb_partsdroplist->set_datasetpath(partsdroplist.datasetpath);
  for (auto part : partsdroplist.partitions) {
    pb_partsdroplist->add_partitions(part);
  }
  return arrow::Status::OK();
}

// HeartbeatResultCmd
arrow::Status FromProto(const pb::HeartbeatResultCmd& pb_resultcmd, HeartbeatResultCmd* resultcmd) {
  if (pb_resultcmd.hbrc_action() == pb::HeartbeatResultCmd::NOACTION) {
    resultcmd->hbrc_action = HeartbeatResultCmd::NOACTION;
  } else if (pb_resultcmd.hbrc_action() == pb::HeartbeatResultCmd::DROPCACHE) {
    resultcmd->hbrc_action = HeartbeatResultCmd::DROPCACHE;
  } else {
    resultcmd->hbrc_action = HeartbeatResultCmd::UNKNOWN;
  }
  resultcmd->hbrc_parameters.resize(pb_resultcmd.hbrc_parameters_size());
  for (int i=0; i<pb_resultcmd.hbrc_parameters_size(); i++) {
    FromProto(pb_resultcmd.hbrc_parameters(i), &(resultcmd->hbrc_parameters[i]));
  }

  return arrow::Status::OK();
}

arrow::Status ToProto(const HeartbeatResultCmd& resultcmd, pb::HeartbeatResultCmd* pb_resultcmd) {
  if (resultcmd.hbrc_action == HeartbeatResultCmd::NOACTION) {
    pb_resultcmd->set_hbrc_action(pb::HeartbeatResultCmd::NOACTION);
  } else if (resultcmd.hbrc_action == HeartbeatResultCmd::DROPCACHE) {
    pb_resultcmd->set_hbrc_action(pb::HeartbeatResultCmd::DROPCACHE);
  } else {
    pb_resultcmd->set_hbrc_action(pb::HeartbeatResultCmd::UNKNOWN);
  }
  pb_resultcmd->clear_hbrc_parameters();
  for (auto partlistofds : resultcmd.hbrc_parameters) {
    ToProto(partlistofds, pb_resultcmd->add_hbrc_parameters());
  }

  return arrow::Status::OK();
}

// HeartbeatResult
arrow::Status FromProto(const pb::HeartbeatResult& pb_result, HeartbeatResult* result) {
  if (pb_result.result_code() == pb::HeartbeatResult::REGISTERED) {
    result->result_code = HeartbeatResult::REGISTERED;
  } else if (pb_result.result_code() == pb::HeartbeatResult::HEARTBEATED) {
    result->result_code = HeartbeatResult::HEARTBEATED;
  } else {
    result->result_code = HeartbeatResult::UNKNOWN;
  }

  if (pb_result.result_hascmd() == true) {
    result->result_hascmd = true;
    FromProto(pb_result.result_command(), &(result->result_command));
  } else {
    result->result_hascmd = false;
  }

  return arrow::Status::OK();
}

arrow::Status ToProto(const HeartbeatResult& result, pb::HeartbeatResult* pb_result) {
  if (result.result_code == HeartbeatResult::REGISTERED) {
    pb_result->set_result_code(pb::HeartbeatResult::REGISTERED);
  } else if (result.result_code == HeartbeatResult::HEARTBEATED) {
    pb_result->set_result_code(pb::HeartbeatResult::HEARTBEATED);
  } else {
    pb_result->set_result_code(pb::HeartbeatResult::UNKNOWN);
  }

  if (result.result_hascmd == true) {
    pb_result->set_result_hascmd(true);
    ToProto(result.result_command, pb_result->mutable_result_command());
  } else {
    pb_result->set_result_hascmd(false);
  }

  return arrow::Status::OK();
}

arrow::Status FromProto(const pb::LocalColumnInfo& pb_info, LocalColumnInfo* info) {
  info->column_index = pb_info.column_index();
  info->data_offset = pb_info.data_offset();
  info->data_size = pb_info.data_size();
  info->mmap_fd = pb_info.mmap_fd();
  info->mmap_size = pb_info.mmap_size();
  return arrow::Status::OK();
}

arrow::Status FromProto(const pb::LocalPartitionInfo& pb_info, LocalPartitionInfo* info) {
  info->columns.resize(pb_info.columninfo_size());
  for (int i = 0; i < pb_info.columninfo_size(); ++i) {
    RETURN_NOT_OK(FromProto(pb_info.columninfo(i), &info->columns[i]));
  }
  return arrow::Status::OK();
}

arrow::Status FromProto(const pb::LocalReleaseResult& pb_result, LocalReleaseResult* result) {
  result->result_code = pb_result.result_code();
  return arrow::Status::OK();
}

arrow::Status ToProto(const LocalColumnInfo& info, pb::LocalColumnInfo* pb_info) {
  pb_info->set_column_index(info.column_index);
  pb_info->set_data_offset(info.data_offset);
  pb_info->set_data_size(info.data_size);
  pb_info->set_mmap_fd(info.mmap_fd);
  pb_info->set_mmap_size(info.mmap_size);
  return arrow::Status::OK();
}

arrow::Status ToProto(const LocalPartitionInfo& info, pb::LocalPartitionInfo* pb_info) {
  pb_info->clear_columninfo();
  for (const LocalColumnInfo& column : info.columns) {
    ToProto(column, pb_info->add_columninfo());
  }
  return arrow::Status::OK();
}

arrow::Status ToProto(const LocalReleaseResult& result, pb::LocalReleaseResult* pb_result) {
  pb_result->set_result_code(result.result_code);
  return arrow::Status::OK();
}

}  // namespace internal
}  // namespace rpc
}  // namespace pegasus
