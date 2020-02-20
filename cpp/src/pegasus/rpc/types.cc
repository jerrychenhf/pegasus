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

#include "rpc/types.h"

#include <memory>
#include <sstream>
#include <utility>

#include "rpc/serialization_internal.h"
#include "arrow/io/memory.h"
#include "arrow/ipc/dictionary.h"
#include "arrow/ipc/reader.h"
#include "arrow/status.h"
#include "arrow/table.h"
#include "arrow/util/uri.h"

namespace pegasus {
namespace rpc {

const char* kSchemeGrpc = "grpc";
const char* kSchemeGrpcTcp = "grpc+tcp";
const char* kSchemeGrpcUnix = "grpc+unix";
const char* kSchemeGrpcTls = "grpc+tls";

const char* kErrorDetailTypeId = "pegasus::FlightStatusDetail";

const char* FlightStatusDetail::type_id() const { return kErrorDetailTypeId; }

std::string FlightStatusDetail::ToString() const { return CodeAsString(); }

FlightStatusCode FlightStatusDetail::code() const { return code_; }

std::string FlightStatusDetail::CodeAsString() const {
  switch (code()) {
    case FlightStatusCode::Internal:
      return "Internal";
    case FlightStatusCode::TimedOut:
      return "TimedOut";
    case FlightStatusCode::Cancelled:
      return "Cancelled";
    case FlightStatusCode::Unauthenticated:
      return "Unauthenticated";
    case FlightStatusCode::Unauthorized:
      return "Unauthorized";
    case FlightStatusCode::Unavailable:
      return "Unavailable";
    default:
      return "Unknown";
  }
}

std::shared_ptr<FlightStatusDetail> FlightStatusDetail::UnwrapStatus(
    const arrow::Status& status) {
  if (!status.detail() || status.detail()->type_id() != kErrorDetailTypeId) {
    return nullptr;
  }
  return std::dynamic_pointer_cast<FlightStatusDetail>(status.detail());
}

arrow::Status MakeFlightError(FlightStatusCode code, const std::string& message) {
  arrow::StatusCode arrow_code = arrow::StatusCode::IOError;
  return arrow::Status(arrow_code, message, std::make_shared<FlightStatusDetail>(code));
}

bool Option::Equals(const Option& other) const {
  return key == other.key && value == other.value;
}

bool FlightDescriptor::Equals(const FlightDescriptor& other) const {
  if (type != other.type) {
    return false;
  }
  switch (type) {
    case PATH:
      return path == other.path && options == other.options;
    case CMD:
      return cmd == other.cmd && options == other.options;
    default:
      return false;
  }
}

std::string FlightDescriptor::ToString() const {
  std::stringstream ss;
  ss << "FlightDescriptor<";
  switch (type) {
    case PATH: {
      bool first = true;
      ss << "path = '";
      for (const auto& p : path) {
        if (!first) {
          ss << "/";
        }
        first = false;
        ss << p;
      }
      ss << "'";
      break;
    }
    case CMD:
      ss << "cmd = '" << cmd << "'";
      break;
    default:
      break;
  }
  ss << ">";
  return ss.str();
}

arrow::Status SchemaResult::GetSchema(arrow::ipc::DictionaryMemo* dictionary_memo,
                               std::shared_ptr<arrow::Schema>* out) const {
  arrow::io::BufferReader schema_reader(raw_schema_);
  RETURN_NOT_OK(arrow::ipc::ReadSchema(&schema_reader, dictionary_memo, out));
  return arrow::Status::OK();
}

arrow::Status FlightDescriptor::SerializeToString(std::string* out) const {
  pb::FlightDescriptor pb_descriptor;
  RETURN_NOT_OK(internal::ToProto(*this, &pb_descriptor));

  if (!pb_descriptor.SerializeToString(out)) {
    return arrow::Status::IOError("Serialized descriptor exceeded 2 GiB limit");
  }
  return arrow::Status::OK();
}

arrow::Status FlightDescriptor::Deserialize(const std::string& serialized,
                                     FlightDescriptor* out) {
  pb::FlightDescriptor pb_descriptor;
  if (!pb_descriptor.ParseFromString(serialized)) {
    return arrow::Status::Invalid("Not a valid descriptor");
  }
  return internal::FromProto(pb_descriptor, out);
}

bool Ticket::Equals(const Ticket& other) const { return ticket == other.ticket; }

arrow::Status Ticket::SerializeToString(std::string* out) const {
  pb::Ticket pb_ticket;
  internal::ToProto(*this, &pb_ticket);

  if (!pb_ticket.SerializeToString(out)) {
    return arrow::Status::IOError("Serialized ticket exceeded 2 GiB limit");
  }
  return arrow::Status::OK();
}

arrow::Status Ticket::Deserialize(const std::string& serialized, Ticket* out) {
  pb::Ticket pb_ticket;
  if (!pb_ticket.ParseFromString(serialized)) {
    return arrow::Status::Invalid("Not a valid ticket");
  }
  return internal::FromProto(pb_ticket, out);
}

arrow::Status FlightInfo::GetSchema(arrow::ipc::DictionaryMemo* dictionary_memo,
                             std::shared_ptr<arrow::Schema>* out) const {
  if (reconstructed_schema_) {
    *out = schema_;
    return arrow::Status::OK();
  }
  arrow::io::BufferReader schema_reader(data_.schema);
  RETURN_NOT_OK(arrow::ipc::ReadSchema(&schema_reader, dictionary_memo, &schema_));
  reconstructed_schema_ = true;
  *out = schema_;
  return arrow::Status::OK();
}

arrow::Status FlightInfo::SerializeToString(std::string* out) const {
  pb::FlightInfo pb_info;
  RETURN_NOT_OK(internal::ToProto(*this, &pb_info));

  if (!pb_info.SerializeToString(out)) {
    return arrow::Status::IOError("Serialized FlightInfo exceeded 2 GiB limit");
  }
  return arrow::Status::OK();
}

arrow::Status FlightInfo::Deserialize(const std::string& serialized,
                               std::unique_ptr<FlightInfo>* out) {
  pb::FlightInfo pb_info;
  if (!pb_info.ParseFromString(serialized)) {
    return arrow::Status::Invalid("Not a valid FlightInfo");
  }
  FlightInfo::Data data;
  RETURN_NOT_OK(internal::FromProto(pb_info, &data));
  out->reset(new FlightInfo(data));
  return arrow::Status::OK();
}

Location::Location() { uri_ = std::make_shared<arrow::internal::Uri>(); }

arrow::Status Location::Parse(const std::string& uri_string, Location* location) {
  return location->uri_->Parse(uri_string);
}

arrow::Status Location::ForGrpcTcp(const std::string& host, const int port, Location* location) {
  std::stringstream uri_string;
  uri_string << "grpc+tcp://" << host << ':' << port;
  return Location::Parse(uri_string.str(), location);
}

arrow::Status Location::ForGrpcTls(const std::string& host, const int port, Location* location) {
  std::stringstream uri_string;
  uri_string << "grpc+tls://" << host << ':' << port;
  return Location::Parse(uri_string.str(), location);
}

arrow::Status Location::ForGrpcUnix(const std::string& path, Location* location) {
  std::stringstream uri_string;
  uri_string << "grpc+unix://" << path;
  return Location::Parse(uri_string.str(), location);
}

std::string Location::ToString() const { return uri_->ToString(); }
std::string Location::scheme() const {
  std::string scheme = uri_->scheme();
  if (scheme.empty()) {
    // Default to grpc+tcp
    return "grpc+tcp";
  }
  return scheme;
}

std::string Location::host() const {
  return uri_->host(); 
}

int32_t Location::port() const {
  return uri_->port(); 
}

bool Location::Equals(const Location& other) const {
  return ToString() == other.ToString();
}

bool FlightEndpoint::Equals(const FlightEndpoint& other) const {
  return ticket == other.ticket && locations == other.locations;
}

arrow::Status MetadataRecordBatchReader::ReadAll(
    std::vector<std::shared_ptr<arrow::RecordBatch>>* batches) {
  FlightStreamChunk chunk;

  while (true) {
    RETURN_NOT_OK(Next(&chunk));
    if (!chunk.data) break;
    batches->emplace_back(std::move(chunk.data));
  }
  return arrow::Status::OK();
}

arrow::Status MetadataRecordBatchReader::ReadAll(std::shared_ptr<arrow::Table>* table) {
  std::vector<std::shared_ptr<arrow::RecordBatch>> batches;
  RETURN_NOT_OK(ReadAll(&batches));
  return arrow::Table::FromRecordBatches(schema(), batches, table);
}

SimpleFlightListing::SimpleFlightListing(const std::vector<FlightInfo>& flights)
    : position_(0), flights_(flights) {}

SimpleFlightListing::SimpleFlightListing(std::vector<FlightInfo>&& flights)
    : position_(0), flights_(std::move(flights)) {}

arrow::Status SimpleFlightListing::Next(std::unique_ptr<FlightInfo>* info) {
  if (position_ >= static_cast<int>(flights_.size())) {
    *info = nullptr;
    return arrow::Status::OK();
  }
  *info = std::unique_ptr<FlightInfo>(new FlightInfo(std::move(flights_[position_++])));
  return arrow::Status::OK();
}

SimpleResultStream::SimpleResultStream(std::vector<Result>&& results)
    : results_(std::move(results)), position_(0) {}

arrow::Status SimpleResultStream::Next(std::unique_ptr<Result>* result) {
  if (position_ >= results_.size()) {
    *result = nullptr;
    return arrow::Status::OK();
  }
  *result = std::unique_ptr<Result>(new Result(std::move(results_[position_++])));
  return arrow::Status::OK();
}

arrow::Status BasicAuth::Deserialize(const std::string& serialized, BasicAuth* out) {
  pb::BasicAuth pb_result;
  pb_result.ParseFromString(serialized);
  return internal::FromProto(pb_result, out);
}

arrow::Status BasicAuth::Serialize(const BasicAuth& basic_auth, std::string* out) {
  pb::BasicAuth pb_result;
  RETURN_NOT_OK(internal::ToProto(basic_auth, &pb_result));
  *out = pb_result.SerializeAsString();
  return arrow::Status::OK();
}

bool NodeInfo::Equals(const NodeInfo& other) const {
  if (cache_capacity != other.cache_capacity ||
    cache_free != other.cache_free) {
    return false;
  }
  
  return true;
}

bool HeartbeatInfo::Equals(const HeartbeatInfo& other) const {
  if (type != other.type) {
    return false;
  }
  
  if (hostname != other.hostname) {
    return false;
  }
  
  switch (type) {
    case REGISTRATION: {
      if (address == nullptr && other.address == nullptr)
        return true;
      else if(address == nullptr || other.address == nullptr)
        return false;
    
      return *(address.get()) == *(other.address.get());
    }
    case HEARTBEAT: {
      break;
    }
    default:
      return true;
  }
  
  if (node_info == nullptr && other.node_info == nullptr)
    return true;
  else if(node_info == nullptr || other.node_info == nullptr)
    return false;

  return *(node_info.get()) == *(other.node_info.get());
}

bool HeartbeatResult::Equals(const HeartbeatResult& other) const {
  if (result_code != other.result_code) {
    return false;
  }
  return true;
}

}  // namespace rpc
}  // namespace pegasus
