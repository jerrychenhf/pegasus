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

#pragma once

#include <memory>
#include <string>

#include "rpc/protocol_internal.h"  // IWYU pragma: keep
#include "rpc/types.h"
#include "arrow/util/macros.h"

namespace grpc {

class Status;

}  // namespace grpc

namespace arrow {

class Status;

namespace ipc {

class Message;

}  // namespace ipc
}  // namespace arrow

namespace pegasus {

class Schema;

namespace pb = pegasus::rpc::protocol;

namespace rpc {

#define GRPC_RETURN_NOT_OK(expr)                          \
  do {                                                    \
    arrow::Status _s = (expr);                          \
    if (ARROW_PREDICT_FALSE(!_s.ok())) {                  \
      return pegasus::rpc::internal::ToGrpcStatus(_s); \
    }                                                     \
  } while (0)

#define GRPC_RETURN_NOT_GRPC_OK(expr)    \
  do {                                   \
    ::grpc::Status _s = (expr);          \
    if (ARROW_PREDICT_FALSE(!_s.ok())) { \
      return _s;                         \
    }                                    \
  } while (0)

namespace internal {

/// The name of the header used to pass authentication tokens.
PEGASUS_RPC_EXPORT
extern const char* kGrpcAuthHeader;

PEGASUS_RPC_EXPORT
arrow::Status SchemaToString(const arrow::Schema& schema, std::string* out);

PEGASUS_RPC_EXPORT
arrow::Status FromGrpcStatus(const ::grpc::Status& grpc_status);

PEGASUS_RPC_EXPORT
::grpc::Status ToGrpcStatus(const arrow::Status& arrow_status);

// These functions depend on protobuf types which are not exported in the Flight DLL.

arrow::Status FromProto(const pb::ActionType& pb_type, ActionType* type);
arrow::Status FromProto(const pb::Action& pb_action, Action* action);
arrow::Status FromProto(const pb::Result& pb_result, Result* result);
arrow::Status FromProto(const pb::Criteria& pb_criteria, Criteria* criteria);
arrow::Status FromProto(const pb::Location& pb_location, Location* location);
arrow::Status FromProto(const pb::Ticket& pb_ticket, Ticket* ticket);
arrow::Status FromProto(const pb::FlightData& pb_data, FlightDescriptor* descriptor,
                 std::unique_ptr<arrow::ipc::Message>* message);
arrow::Status FromProto(const pb::FlightDescriptor& pb_descr, FlightDescriptor* descr);
arrow::Status FromProto(const pb::FlightEndpoint& pb_endpoint, FlightEndpoint* endpoint);
arrow::Status FromProto(const pb::FlightInfo& pb_info, FlightInfo::Data* info);
arrow::Status FromProto(const pb::SchemaResult& pb_result, std::string* result);
arrow::Status FromProto(const pb::BasicAuth& pb_basic_auth, BasicAuth* info);
arrow::Status FromProto(const pb::HeartbeatInfo& pb_info, HeartbeatInfo* info);

arrow::Status ToProto(const FlightDescriptor& descr, pb::FlightDescriptor* pb_descr);
arrow::Status ToProto(const FlightInfo& info, pb::FlightInfo* pb_info);
arrow::Status ToProto(const ActionType& type, pb::ActionType* pb_type);
arrow::Status ToProto(const Action& action, pb::Action* pb_action);
arrow::Status ToProto(const Result& result, pb::Result* pb_result);
arrow::Status ToProto(const SchemaResult& result, pb::SchemaResult* pb_result);
void ToProto(const Ticket& ticket, pb::Ticket* pb_ticket);
arrow::Status ToProto(const BasicAuth& basic_auth, pb::BasicAuth* pb_basic_auth);
arrow::Status ToProto(const HeartbeatResult& result, pb::HeartbeatResult* pb_result);
}  // namespace internal
}  // namespace rpc
}  // namespace pegasus
