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

// Platform-specific defines
#include "rpc/platform.h"

#include "rpc/server.h"

#include <signal.h>
#include <atomic>
#include <cstdint>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>

#ifdef GRPCPP_PP_INCLUDE
#include <grpcpp/grpcpp.h>
#else
#include <grpc++/grpc++.h>
#endif

#include "arrow/buffer.h"
#include "arrow/ipc/dictionary.h"
#include "arrow/ipc/options.h"
#include "arrow/ipc/reader.h"
#include "arrow/ipc/writer.h"
#include "arrow/memory_pool.h"
#include "arrow/record_batch.h"
#include "arrow/status.h"
#include "arrow/util/io_util.h"
#include "arrow/util/logging.h"
#include "arrow/util/uri.h"

#include "rpc/internal.h"
#include "rpc/middleware.h"
#include "rpc/middleware_internal.h"
#include "rpc/serialization_internal.h"
#include "rpc/server_auth.h"
#include "rpc/server_middleware.h"
#include "rpc/types.h"

#include "rpc/file_batch_reader.h"

using FlightService = pegasus::rpc::protocol::FlightService;
using ServerContext = grpc::ServerContext;

template <typename T>
using ServerWriter = grpc::ServerWriter<T>;

namespace pb = pegasus::rpc::protocol;

namespace pegasus {
namespace rpc {

// Macro that runs interceptors before returning the given status
#define RETURN_WITH_MIDDLEWARE(CONTEXT, STATUS) \
  do {                                          \
    const auto& __s = (STATUS);                 \
    return CONTEXT.FinishRequest(__s);          \
  } while (false)

#define CHECK_ARG_NOT_NULL(CONTEXT, VAL, MESSAGE)                                      \
  if (VAL == nullptr) {                                                                \
    RETURN_WITH_MIDDLEWARE(CONTEXT,                                                    \
                           grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, MESSAGE)); \
  }

// Same as RETURN_NOT_OK, but accepts either Arrow or gRPC status, and
// will run interceptors
#define SERVICE_RETURN_NOT_OK(CONTEXT, expr) \
  do {                                       \
    const auto& _s = (expr);                 \
    if (ARROW_PREDICT_FALSE(!_s.ok())) {     \
      return CONTEXT.FinishRequest(_s);      \
    }                                        \
  } while (false)

namespace {

// A MessageReader implementation that reads from a gRPC ServerReader
class FlightIpcMessageReader : public arrow::ipc::MessageReader {
 public:
  explicit FlightIpcMessageReader(
      grpc::ServerReaderWriter<pb::PutResult, pb::FlightData>* reader,
      std::shared_ptr<arrow::Buffer>* last_metadata)
      : reader_(reader), app_metadata_(last_metadata) {}

  arrow::Status ReadNextMessage(std::unique_ptr<arrow::ipc::Message>* out) override {
    if (stream_finished_) {
      *out = nullptr;
      *app_metadata_ = nullptr;
      return arrow::Status::OK();
    }
    internal::FlightData data;
    if (!internal::ReadPayload(reader_, &data)) {
      // Stream is finished
      stream_finished_ = true;
      if (first_message_) {
        return arrow::Status::Invalid(
            "Client provided malformed message or did not provide message");
      }
      *out = nullptr;
      *app_metadata_ = nullptr;
      return arrow::Status::OK();
    }

    if (first_message_) {
      if (!data.descriptor) {
        return arrow::Status::Invalid("DoPut must start with non-null descriptor");
      }
      descriptor_ = *data.descriptor;
      first_message_ = false;
    }

    RETURN_NOT_OK(data.OpenMessage(out));
    *app_metadata_ = std::move(data.app_metadata);
    return arrow::Status::OK();
  }

  const FlightDescriptor& descriptor() const { return descriptor_; }

 protected:
  grpc::ServerReaderWriter<pb::PutResult, pb::FlightData>* reader_;
  bool stream_finished_ = false;
  bool first_message_ = true;
  FlightDescriptor descriptor_;
  std::shared_ptr<arrow::Buffer>* app_metadata_;
};

class FlightMessageReaderImpl : public FlightMessageReader {
 public:
  explicit FlightMessageReaderImpl(
      grpc::ServerReaderWriter<pb::PutResult, pb::FlightData>* reader)
      : reader_(reader) {}

  arrow::Status Init() {
    message_reader_ = new FlightIpcMessageReader(reader_, &last_metadata_);
    return arrow::ipc::RecordBatchStreamReader::Open(
        std::unique_ptr<arrow::ipc::MessageReader>(message_reader_), &batch_reader_);
  }

  const FlightDescriptor& descriptor() const override {
    return message_reader_->descriptor();
  }

  std::shared_ptr<arrow::Schema> schema() const override { return batch_reader_->schema(); }

  arrow::Status Next(FlightStreamChunk* out) override {
    out->app_metadata = nullptr;
    RETURN_NOT_OK(batch_reader_->ReadNext(&out->data));
    out->app_metadata = std::move(last_metadata_);
    return arrow::Status::OK();
  }

 private:
  std::shared_ptr<arrow::Schema> schema_;
  std::unique_ptr<arrow::ipc::DictionaryMemo> dictionary_memo_;
  grpc::ServerReaderWriter<pb::PutResult, pb::FlightData>* reader_;
  FlightIpcMessageReader* message_reader_;
  std::shared_ptr<arrow::Buffer> last_metadata_;
  std::shared_ptr<arrow::RecordBatchReader> batch_reader_;
};

class GrpcMetadataWriter : public FlightMetadataWriter {
 public:
  explicit GrpcMetadataWriter(
      grpc::ServerReaderWriter<pb::PutResult, pb::FlightData>* writer)
      : writer_(writer) {}

  arrow::Status WriteMetadata(const arrow::Buffer& buffer) override {
    pb::PutResult message{};
    message.set_app_metadata(buffer.data(), buffer.size());
    if (writer_->Write(message)) {
      return arrow::Status::OK();
    }
    return arrow::Status::IOError("Unknown error writing metadata.");
  }

 private:
  grpc::ServerReaderWriter<pb::PutResult, pb::FlightData>* writer_;
};

class GrpcServerAuthReader : public ServerAuthReader {
 public:
  explicit GrpcServerAuthReader(
      grpc::ServerReaderWriter<pb::HandshakeResponse, pb::HandshakeRequest>* stream)
      : stream_(stream) {}

  arrow::Status Read(std::string* token) override {
    pb::HandshakeRequest request;
    if (stream_->Read(&request)) {
      *token = std::move(*request.mutable_payload());
      return arrow::Status::OK();
    }
    return arrow::Status::IOError("Stream is closed.");
  }

 private:
  grpc::ServerReaderWriter<pb::HandshakeResponse, pb::HandshakeRequest>* stream_;
};

class GrpcServerAuthSender : public ServerAuthSender {
 public:
  explicit GrpcServerAuthSender(
      grpc::ServerReaderWriter<pb::HandshakeResponse, pb::HandshakeRequest>* stream)
      : stream_(stream) {}

  arrow::Status Write(const std::string& token) override {
    pb::HandshakeResponse response;
    response.set_payload(token);
    if (stream_->Write(response)) {
      return arrow::Status::OK();
    }
    return arrow::Status::IOError("Stream was closed.");
  }

 private:
  grpc::ServerReaderWriter<pb::HandshakeResponse, pb::HandshakeRequest>* stream_;
};

class FlightServiceImpl;
class GrpcServerCallContext : public ServerCallContext {
  const std::string& peer_identity() const override { return peer_identity_; }

  // Helper method that runs interceptors given the result of an RPC,
  // then returns the final gRPC status to send to the client
  grpc::Status FinishRequest(const grpc::Status& status) {
    // Don't double-convert status - return the original one here
    FinishRequest(internal::FromGrpcStatus(status));
    return status;
  }

  grpc::Status FinishRequest(const arrow::Status& status) {
    for (const auto& instance : middleware_) {
      instance->CallCompleted(status);
    }
    return internal::ToGrpcStatus(status);
  }

  ServerMiddleware* GetMiddleware(const std::string& key) const override {
    const auto& instance = middleware_map_.find(key);
    if (instance == middleware_map_.end()) {
      return nullptr;
    }
    return instance->second.get();
  }

 private:
  friend class FlightServiceImpl;
  ServerContext* context_;
  std::string peer_identity_;
  std::vector<std::shared_ptr<ServerMiddleware>> middleware_;
  std::unordered_map<std::string, std::shared_ptr<ServerMiddleware>> middleware_map_;
};

class GrpcAddCallHeaders : public AddCallHeaders {
 public:
  explicit GrpcAddCallHeaders(grpc::ServerContext* context) : context_(context) {}
  ~GrpcAddCallHeaders() override = default;

  void AddHeader(const std::string& key, const std::string& value) override {
    context_->AddInitialMetadata(key, value);
  }

 private:
  grpc::ServerContext* context_;
};

// This class glues an implementation of FlightServerBase together with the
// gRPC service definition, so the latter is not exposed in the public API
class FlightServiceImpl : public FlightService::Service {
 public:
  explicit FlightServiceImpl(
      std::shared_ptr<ServerAuthHandler> auth_handler,
      std::vector<std::pair<std::string, std::shared_ptr<ServerMiddlewareFactory>>>
          middleware,
      FlightServerBase* server)
      : auth_handler_(auth_handler), middleware_(middleware), server_(server) {}

  template <typename UserType, typename Iterator, typename ProtoType>
  grpc::Status WriteStream(Iterator* iterator, ServerWriter<ProtoType>* writer) {
    if (!iterator) {
      return grpc::Status(grpc::StatusCode::INTERNAL, "No items to iterate");
    }
    // Write flight info to stream until listing is exhausted
    while (true) {
      ProtoType pb_value;
      std::unique_ptr<UserType> value;
      GRPC_RETURN_NOT_OK(iterator->Next(&value));
      if (!value) {
        break;
      }
      GRPC_RETURN_NOT_OK(internal::ToProto(*value, &pb_value));

      // Blocking write
      if (!writer->Write(pb_value)) {
        // Write returns false if the stream is closed
        break;
      }
    }
    return grpc::Status::OK;
  }

  template <typename UserType, typename ProtoType>
  grpc::Status WriteStream(const std::vector<UserType>& values,
                           ServerWriter<ProtoType>* writer) {
    // Write flight info to stream until listing is exhausted
    for (const UserType& value : values) {
      ProtoType pb_value;
      GRPC_RETURN_NOT_OK(internal::ToProto(value, &pb_value));
      // Blocking write
      if (!writer->Write(pb_value)) {
        // Write returns false if the stream is closed
        break;
      }
    }
    return grpc::Status::OK;
  }

  // Authenticate the client (if applicable) and construct the call context
  grpc::Status CheckAuth(const FlightMethod& method, ServerContext* context,
                         GrpcServerCallContext& flight_context) {
    flight_context.context_ = context;
    if (!auth_handler_) {
      flight_context.peer_identity_ = "";
    } else {
      const auto client_metadata = context->client_metadata();
      const auto auth_header = client_metadata.find(internal::kGrpcAuthHeader);
      std::string token;
      if (auth_header == client_metadata.end()) {
        token = "";
      } else {
        token = std::string(auth_header->second.data(), auth_header->second.length());
      }
      GRPC_RETURN_NOT_OK(auth_handler_->IsValid(token, &flight_context.peer_identity_));
    }

    return MakeCallContext(method, context, flight_context);
  }

  // Authenticate the client (if applicable) and construct the call context
  grpc::Status MakeCallContext(const FlightMethod& method, ServerContext* context,
                               GrpcServerCallContext& flight_context) {
    // Run server middleware
    const CallInfo info{method};
    CallHeaders incoming_headers;
    for (const auto& entry : context->client_metadata()) {
      incoming_headers.insert(
          {arrow::util::string_view(entry.first.data(), entry.first.length()),
           arrow::util::string_view(entry.second.data(), entry.second.length())});
    }

    GrpcAddCallHeaders outgoing_headers(context);
    for (const auto& factory : middleware_) {
      std::shared_ptr<ServerMiddleware> instance;
      arrow::Status result = factory.second->StartCall(info, incoming_headers, &instance);
      if (!result.ok()) {
        // Interceptor rejected call, end the request on all existing
        // interceptors
        return flight_context.FinishRequest(result);
      }
      if (instance != nullptr) {
        flight_context.middleware_.push_back(instance);
        flight_context.middleware_map_.insert({factory.first, instance});
        instance->SendingHeaders(&outgoing_headers);
      }
    }

    return grpc::Status::OK;
  }

  grpc::Status Handshake(
      ServerContext* context,
      grpc::ServerReaderWriter<pb::HandshakeResponse, pb::HandshakeRequest>* stream) {
    GrpcServerCallContext flight_context;
    GRPC_RETURN_NOT_GRPC_OK(
        MakeCallContext(FlightMethod::Handshake, context, flight_context));

    if (!auth_handler_) {
      RETURN_WITH_MIDDLEWARE(
          flight_context,
          grpc::Status(
              grpc::StatusCode::UNIMPLEMENTED,
              "This service does not have an authentication mechanism enabled."));
    }
    GrpcServerAuthSender outgoing{stream};
    GrpcServerAuthReader incoming{stream};
    RETURN_WITH_MIDDLEWARE(flight_context,
                           auth_handler_->Authenticate(&outgoing, &incoming));
  }

  grpc::Status ListFlights(ServerContext* context, const pb::Criteria* request,
                           ServerWriter<pb::FlightInfo>* writer) {
    GrpcServerCallContext flight_context;
    GRPC_RETURN_NOT_GRPC_OK(
        CheckAuth(FlightMethod::ListFlights, context, flight_context));

    // Retrieve the listing from the implementation
    std::unique_ptr<FlightListing> listing;

    Criteria criteria;
    if (request) {
      SERVICE_RETURN_NOT_OK(flight_context, internal::FromProto(*request, &criteria));
    }
    SERVICE_RETURN_NOT_OK(flight_context,
                          server_->ListFlights(flight_context, &criteria, &listing));
    if (!listing) {
      // Treat null listing as no flights available
      RETURN_WITH_MIDDLEWARE(flight_context, grpc::Status::OK);
    }
    RETURN_WITH_MIDDLEWARE(flight_context,
                           WriteStream<FlightInfo>(listing.get(), writer));
  }

  grpc::Status GetFlightInfo(ServerContext* context, const pb::FlightDescriptor* request,
                             pb::FlightInfo* response) {
    GrpcServerCallContext flight_context;
    GRPC_RETURN_NOT_GRPC_OK(
        CheckAuth(FlightMethod::GetFlightInfo, context, flight_context));

    CHECK_ARG_NOT_NULL(flight_context, request, "FlightDescriptor cannot be null");

    FlightDescriptor descr;
    SERVICE_RETURN_NOT_OK(flight_context, internal::FromProto(*request, &descr));

    std::unique_ptr<FlightInfo> info;
    SERVICE_RETURN_NOT_OK(flight_context,
                          server_->GetFlightInfo(flight_context, descr, &info));

    if (!info) {
      // Treat null listing as no flights available
      RETURN_WITH_MIDDLEWARE(
          flight_context, grpc::Status(grpc::StatusCode::NOT_FOUND, "Flight not found"));
    }

    SERVICE_RETURN_NOT_OK(flight_context, internal::ToProto(*info, response));
    RETURN_WITH_MIDDLEWARE(flight_context, grpc::Status::OK);
  }

  grpc::Status GetSchema(ServerContext* context, const pb::FlightDescriptor* request,
                         pb::SchemaResult* response) {
    GrpcServerCallContext flight_context;
    GRPC_RETURN_NOT_GRPC_OK(CheckAuth(FlightMethod::GetSchema, context, flight_context));

    CHECK_ARG_NOT_NULL(flight_context, request, "FlightDescriptor cannot be null");

    FlightDescriptor descr;
    SERVICE_RETURN_NOT_OK(flight_context, internal::FromProto(*request, &descr));

    std::unique_ptr<SchemaResult> result;
    SERVICE_RETURN_NOT_OK(flight_context,
                          server_->GetSchema(flight_context, descr, &result));

    if (!result) {
      // Treat null listing as no flights available
      RETURN_WITH_MIDDLEWARE(
          flight_context, grpc::Status(grpc::StatusCode::NOT_FOUND, "Flight not found"));
    }

    SERVICE_RETURN_NOT_OK(flight_context, internal::ToProto(*result, response));
    RETURN_WITH_MIDDLEWARE(flight_context, grpc::Status::OK);
  }

  grpc::Status DoGet(ServerContext* context, const pb::Ticket* request,
                     ServerWriter<pb::FlightData>* writer) {
    GrpcServerCallContext flight_context;
    GRPC_RETURN_NOT_GRPC_OK(CheckAuth(FlightMethod::DoGet, context, flight_context));

    CHECK_ARG_NOT_NULL(flight_context, request, "ticket cannot be null");

    Ticket ticket;
    SERVICE_RETURN_NOT_OK(flight_context, internal::FromProto(*request, &ticket));

    arrow::ipc::DictionaryMemo dict_memo;
    SERVICE_RETURN_NOT_OK(flight_context, ticket.DeserializeSchema(&dict_memo));

    std::unique_ptr<FlightDataStream> data_stream;
    SERVICE_RETURN_NOT_OK(flight_context,
                          server_->DoGet(flight_context, ticket, &data_stream));

    if (!data_stream) {
      RETURN_WITH_MIDDLEWARE(flight_context, grpc::Status(grpc::StatusCode::NOT_FOUND,
                                                          "No data in this flight"));
    }

    // Write the schema as the first message in the stream
    FlightPayload schema_payload;
    SERVICE_RETURN_NOT_OK(flight_context, data_stream->GetSchemaPayload(&schema_payload));
    if (!internal::WritePayload(schema_payload, writer)) {
      // Connection terminated?  XXX return error code?
      RETURN_WITH_MIDDLEWARE(flight_context, grpc::Status::OK);
    }

    // Consume data stream and write out payloads
    while (true) {
      FlightPayload payload;
      SERVICE_RETURN_NOT_OK(flight_context, data_stream->Next(&payload));
      if (payload.ipc_message.metadata == nullptr ||
          !internal::WritePayload(payload, writer))
        // No more messages to write, or connection terminated for some other
        // reason
        break;
    }
    RETURN_WITH_MIDDLEWARE(flight_context, grpc::Status::OK);
  }

  grpc::Status DoPut(ServerContext* context,
                     grpc::ServerReaderWriter<pb::PutResult, pb::FlightData>* reader) {
    GrpcServerCallContext flight_context;
    GRPC_RETURN_NOT_GRPC_OK(CheckAuth(FlightMethod::DoPut, context, flight_context));

    auto message_reader =
        std::unique_ptr<FlightMessageReaderImpl>(new FlightMessageReaderImpl(reader));
    SERVICE_RETURN_NOT_OK(flight_context, message_reader->Init());
    auto metadata_writer =
        std::unique_ptr<FlightMetadataWriter>(new GrpcMetadataWriter(reader));
    RETURN_WITH_MIDDLEWARE(flight_context,
                           server_->DoPut(flight_context, std::move(message_reader),
                                          std::move(metadata_writer)));
  }

  grpc::Status ListActions(ServerContext* context, const pb::Empty* request,
                           ServerWriter<pb::ActionType>* writer) {
    GrpcServerCallContext flight_context;
    GRPC_RETURN_NOT_GRPC_OK(
        CheckAuth(FlightMethod::ListActions, context, flight_context));
    // Retrieve the listing from the implementation
    std::vector<ActionType> types;
    SERVICE_RETURN_NOT_OK(flight_context, server_->ListActions(flight_context, &types));
    RETURN_WITH_MIDDLEWARE(flight_context, WriteStream<ActionType>(types, writer));
  }

  grpc::Status DoAction(ServerContext* context, const pb::Action* request,
                        ServerWriter<pb::Result>* writer) {
    GrpcServerCallContext flight_context;
    GRPC_RETURN_NOT_GRPC_OK(CheckAuth(FlightMethod::DoAction, context, flight_context));
    CHECK_ARG_NOT_NULL(flight_context, request, "Action cannot be null");
    Action action;
    SERVICE_RETURN_NOT_OK(flight_context, internal::FromProto(*request, &action));

    std::unique_ptr<ResultStream> results;
    SERVICE_RETURN_NOT_OK(flight_context,
                          server_->DoAction(flight_context, action, &results));

    if (!results) {
      RETURN_WITH_MIDDLEWARE(flight_context, grpc::Status::CANCELLED);
    }

    while (true) {
      std::unique_ptr<Result> result;
      SERVICE_RETURN_NOT_OK(flight_context, results->Next(&result));
      if (!result) {
        // No more results
        break;
      }
      pb::Result pb_result;
      SERVICE_RETURN_NOT_OK(flight_context, internal::ToProto(*result, &pb_result));
      if (!writer->Write(pb_result)) {
        // Stream may be closed
        break;
      }
    }
    RETURN_WITH_MIDDLEWARE(flight_context, grpc::Status::OK);
  }
  
  grpc::Status Heartbeat(ServerContext* context, const pb::HeartbeatInfo* request,
                             pb::HeartbeatResult* response) {
    GrpcServerCallContext flight_context;
    GRPC_RETURN_NOT_GRPC_OK(
        CheckAuth(FlightMethod::Heartbeat, context, flight_context));

    CHECK_ARG_NOT_NULL(flight_context, request, "HeartbeatInfo cannot be null");

    HeartbeatInfo info;
    SERVICE_RETURN_NOT_OK(flight_context, internal::FromProto(*request, &info));

    std::unique_ptr<HeartbeatResult> result;
    SERVICE_RETURN_NOT_OK(flight_context,
                          server_->Heartbeat(flight_context, info, &result));

    SERVICE_RETURN_NOT_OK(flight_context, internal::ToProto(*result, response));
    RETURN_WITH_MIDDLEWARE(flight_context, grpc::Status::OK);
  }
  
  grpc::Status GetLocalData(ServerContext* context, const pb::Ticket* request,
                             pb::LocalPartitionInfo* response) {
    GrpcServerCallContext flight_context;
    GRPC_RETURN_NOT_GRPC_OK(
        CheckAuth(FlightMethod::GetLocalData, context, flight_context));

    CHECK_ARG_NOT_NULL(flight_context, request, "Ticket cannot be null");

    Ticket ticket;
    SERVICE_RETURN_NOT_OK(flight_context, internal::FromProto(*request, &ticket));

    std::unique_ptr<LocalPartitionInfo> result;
    SERVICE_RETURN_NOT_OK(flight_context,
                          server_->GetLocalData(flight_context, ticket, &result));

    SERVICE_RETURN_NOT_OK(flight_context, internal::ToProto(*result, response));
    RETURN_WITH_MIDDLEWARE(flight_context, grpc::Status::OK);
  }
  
  grpc::Status ReleaseLocalData(ServerContext* context, const pb::Ticket* request,
                             pb::LocalReleaseResult* response) {
    GrpcServerCallContext flight_context;
    GRPC_RETURN_NOT_GRPC_OK(
        CheckAuth(FlightMethod::GetLocalData, context, flight_context));

    CHECK_ARG_NOT_NULL(flight_context, request, "Ticket cannot be null");

    Ticket ticket;
    SERVICE_RETURN_NOT_OK(flight_context, internal::FromProto(*request, &ticket));

    std::unique_ptr<LocalReleaseResult> result;
    SERVICE_RETURN_NOT_OK(flight_context,
                          server_->ReleaseLocalData(flight_context, ticket, &result));

    SERVICE_RETURN_NOT_OK(flight_context, internal::ToProto(*result, response));
    RETURN_WITH_MIDDLEWARE(flight_context, grpc::Status::OK);
  }

 private:
  std::shared_ptr<ServerAuthHandler> auth_handler_;
  std::vector<std::pair<std::string, std::shared_ptr<ServerMiddlewareFactory>>>
      middleware_;
  FlightServerBase* server_;
};

}  // namespace

FlightMetadataWriter::~FlightMetadataWriter() = default;

//
// gRPC server lifecycle
//

#if (ATOMIC_INT_LOCK_FREE != 2 || ATOMIC_POINTER_LOCK_FREE != 2)
#error "atomic ints and atomic pointers not always lock-free!"
#endif

using ::arrow::internal::SetSignalHandler;
using ::arrow::internal::SignalHandler;

struct FlightServerBase::Impl {
  std::unique_ptr<FlightServiceImpl> service_;
  std::unique_ptr<grpc::Server> server_;
  int port_;
#ifdef _WIN32
  // Signal handlers are executed in a separate thread on Windows, so getting
  // the current thread instance wouldn't make sense.  This means only a single
  // instance can receive signals on Windows.
  static std::atomic<Impl*> running_instance_;
#else
  static thread_local std::atomic<Impl*> running_instance_;
#endif

  // Signal handling
  std::vector<int> signals_;
  std::vector<SignalHandler> old_signal_handlers_;
  std::atomic<int> got_signal_;

  static void HandleSignal(int signum) {
    auto instance = running_instance_.load();
    if (instance != nullptr) {
      instance->DoHandleSignal(signum);
    }
  }

  void DoHandleSignal(int signum) {
    got_signal_ = signum;
    server_->Shutdown();
  }
};

#ifdef _WIN32
std::atomic<FlightServerBase::Impl*> FlightServerBase::Impl::running_instance_;
#else
thread_local std::atomic<FlightServerBase::Impl*>
    FlightServerBase::Impl::running_instance_;
#endif

FlightServerOptions::FlightServerOptions(const Location& location_)
    : location(location_), auth_handler(nullptr) {}

FlightServerOptions::~FlightServerOptions() = default;

FlightServerBase::FlightServerBase() { impl_.reset(new Impl); }

FlightServerBase::~FlightServerBase() {}

arrow::Status FlightServerBase::Init(const FlightServerOptions& options) {
  impl_->service_.reset(
      new FlightServiceImpl(options.auth_handler, options.middleware, this));

  grpc::ServerBuilder builder;
  // Allow uploading messages of any length
  builder.SetMaxReceiveMessageSize(-1);

  const Location& location = options.location;
  const std::string scheme = location.scheme();
  if (scheme == kSchemeGrpc || scheme == kSchemeGrpcTcp || scheme == kSchemeGrpcTls) {
    std::stringstream address;
    address << location.uri_->host() << ':' << location.uri_->port_text();

    std::shared_ptr<grpc::ServerCredentials> creds;
    if (scheme == kSchemeGrpcTls) {
      grpc::SslServerCredentialsOptions ssl_options;
      for (const auto& pair : options.tls_certificates) {
        ssl_options.pem_key_cert_pairs.push_back({pair.pem_key, pair.pem_cert});
      }
      creds = grpc::SslServerCredentials(ssl_options);
    } else {
      creds = grpc::InsecureServerCredentials();
    }

    builder.AddListeningPort(address.str(), creds, &impl_->port_);
  } else if (scheme == kSchemeGrpcUnix) {
    std::stringstream address;
    address << "unix:" << location.uri_->path();
    builder.AddListeningPort(address.str(), grpc::InsecureServerCredentials());
  } else {
    return arrow::Status::NotImplemented("Scheme is not supported: " + scheme);
  }

  builder.RegisterService(impl_->service_.get());

  // Disable SO_REUSEPORT - it makes debugging/testing a pain as
  // leftover processes can handle requests on accident
  builder.AddChannelArgument(GRPC_ARG_ALLOW_REUSEPORT, 0);

  if (options.builder_hook) {
    options.builder_hook(&builder);
  }

  impl_->server_ = builder.BuildAndStart();
  if (!impl_->server_) {
    return arrow::Status::UnknownError("Server did not start properly");
  }
  return arrow::Status::OK();
}

int FlightServerBase::port() const { return impl_->port_; }

arrow::Status FlightServerBase::SetShutdownOnSignals(const std::vector<int> sigs) {
  impl_->signals_ = sigs;
  impl_->old_signal_handlers_.clear();
  return arrow::Status::OK();
}

arrow::Status FlightServerBase::Serve() {
  if (!impl_->server_) {
    return arrow::Status::UnknownError("Server did not start properly");
  }
  impl_->got_signal_ = 0;
  impl_->old_signal_handlers_.clear();
  impl_->running_instance_ = impl_.get();

  // Override existing signal handlers with our own handler so as to stop the server.
  for (size_t i = 0; i < impl_->signals_.size(); ++i) {
    int signum = impl_->signals_[i];
    SignalHandler new_handler(&Impl::HandleSignal), old_handler;
    ARROW_ASSIGN_OR_RAISE(old_handler, SetSignalHandler(signum, new_handler));
    impl_->old_signal_handlers_.push_back(std::move(old_handler));
  }

  impl_->server_->Wait();
  impl_->running_instance_ = nullptr;

  // Restore signal handlers
  for (size_t i = 0; i < impl_->signals_.size(); ++i) {
    RETURN_NOT_OK(
        SetSignalHandler(impl_->signals_[i], impl_->old_signal_handlers_[i]).status());
  }

  return arrow::Status::OK();
}

int FlightServerBase::GotSignal() const { return impl_->got_signal_; }

arrow::Status FlightServerBase::Shutdown() {
  auto server = impl_->server_.get();
  if (!server) {
    return arrow::Status::Invalid("Shutdown() on uninitialized FlightServerBase");
  }
  impl_->server_->Shutdown();
  return arrow::Status::OK();
}

arrow::Status FlightServerBase::Wait() {
  impl_->server_->Wait();
  impl_->running_instance_ = nullptr;
  return arrow::Status::OK();
}

arrow::Status FlightServerBase::ListFlights(const ServerCallContext& context,
                                     const Criteria* criteria,
                                     std::unique_ptr<FlightListing>* listings) {
  return arrow::Status::NotImplemented("NYI");
}

arrow::Status FlightServerBase::GetFlightInfo(const ServerCallContext& context,
                                       const FlightDescriptor& request,
                                       std::unique_ptr<FlightInfo>* info) {
  return arrow::Status::NotImplemented("NYI");
}

arrow::Status FlightServerBase::DoGet(const ServerCallContext& context, const Ticket& request,
                               std::unique_ptr<FlightDataStream>* data_stream) {
  return arrow::Status::NotImplemented("NYI");
}

arrow::Status FlightServerBase::DoPut(const ServerCallContext& context,
                               std::unique_ptr<FlightMessageReader> reader,
                               std::unique_ptr<FlightMetadataWriter> writer) {
  return arrow::Status::NotImplemented("NYI");
}

arrow::Status FlightServerBase::DoAction(const ServerCallContext& context, const Action& action,
                                  std::unique_ptr<ResultStream>* result) {
  return arrow::Status::NotImplemented("NYI");
}

arrow::Status FlightServerBase::ListActions(const ServerCallContext& context,
                                     std::vector<ActionType>* actions) {
  return arrow::Status::NotImplemented("NYI");
}

arrow::Status FlightServerBase::GetSchema(const ServerCallContext& context,
                                   const FlightDescriptor& request,
                                   std::unique_ptr<SchemaResult>* schema) {
  return arrow::Status::NotImplemented("NYI");
}

arrow::Status FlightServerBase::Heartbeat(const ServerCallContext& context,
                                       const HeartbeatInfo& request,
                                       std::unique_ptr<HeartbeatResult>* response) {
  return arrow::Status::NotImplemented("NYI");
}

arrow::Status FlightServerBase::GetLocalData(const ServerCallContext& context,
                                       const Ticket& request,
                                       std::unique_ptr<LocalPartitionInfo>* response) {
  return arrow::Status::NotImplemented("NYI");
}

arrow::Status FlightServerBase::ReleaseLocalData(const ServerCallContext& context,
                                       const Ticket& request,
                                       std::unique_ptr<LocalReleaseResult>* response) {
  return arrow::Status::NotImplemented("NYI");
}

// ----------------------------------------------------------------------
// Implement RecordBatchStream

class RecordBatchStream::RecordBatchStreamImpl {
 public:
  // Stages of the stream when producing payloads
  enum class Stage {
    NEW,          // The stream has been created, but Next has not been called yet
    DICTIONARY,   // Dictionaries have been collected, and are being sent
    RECORD_BATCH  // Initial have been sent
  };

  RecordBatchStreamImpl(const std::shared_ptr<arrow::RecordBatchReader>& reader,
                        arrow::MemoryPool* pool)
      : pool_(pool), reader_(reader), ipc_options_(arrow::ipc::IpcOptions::Defaults()) {}

  std::shared_ptr<arrow::Schema> schema() { return reader_->schema(); }

  arrow::Status GetSchemaPayload(FlightPayload* payload) {
    return arrow::ipc::internal::GetSchemaPayload(*reader_->schema(), ipc_options_,
                                           &dictionary_memo_, &payload->ipc_message);
  }

  arrow::Status Next(FlightPayload* payload) {
    if (stage_ == Stage::NEW) {
      RETURN_NOT_OK(reader_->ReadNext(&current_batch_));
      if (!current_batch_) {
        // Signal that iteration is over
        payload->ipc_message.metadata = nullptr;
        return arrow::Status::OK();
      }
      RETURN_NOT_OK(CollectDictionaries(*current_batch_));
      stage_ = Stage::DICTIONARY;
    }

    if (stage_ == Stage::DICTIONARY) {
      if (dictionary_index_ == static_cast<int>(dictionaries_.size())) {
        stage_ = Stage::RECORD_BATCH;
        return arrow::ipc::internal::GetRecordBatchPayload(*current_batch_, ipc_options_, pool_,
                                                    &payload->ipc_message);
      } else {
        return GetNextDictionary(payload);
      }
    }

    RETURN_NOT_OK(reader_->ReadNext(&current_batch_));

    // TODO(wesm): Delta dictionaries
    if (!current_batch_) {
      // Signal that iteration is over
      payload->ipc_message.metadata = nullptr;
      return arrow::Status::OK();
    } else {
      return arrow::ipc::internal::GetRecordBatchPayload(*current_batch_, ipc_options_, pool_,
                                                  &payload->ipc_message);
    }
  }

 private:
  arrow::Status GetNextDictionary(FlightPayload* payload) {
    const auto& it = dictionaries_[dictionary_index_++];
    return arrow::ipc::internal::GetDictionaryPayload(it.first, it.second, ipc_options_, pool_,
                                               &payload->ipc_message);
  }

  arrow::Status CollectDictionaries(const arrow::RecordBatch& batch) {
    RETURN_NOT_OK(arrow::ipc::CollectDictionaries(batch, &dictionary_memo_));
    for (auto& pair : dictionary_memo_.id_to_dictionary()) {
      dictionaries_.push_back({pair.first, pair.second});
    }
    return arrow::Status::OK();
  }

  Stage stage_ = Stage::NEW;
  arrow::MemoryPool* pool_;
  std::shared_ptr<arrow::RecordBatchReader> reader_;
  arrow::ipc::DictionaryMemo dictionary_memo_;
  arrow::ipc::IpcOptions ipc_options_;
  std::shared_ptr<arrow::RecordBatch> current_batch_;
  std::vector<std::pair<int64_t, std::shared_ptr<arrow::Array>>> dictionaries_;

  // Index of next dictionary to send
  int dictionary_index_ = 0;
};

FlightDataStream::~FlightDataStream() {}

RecordBatchStream::RecordBatchStream(const std::shared_ptr<arrow::RecordBatchReader>& reader,
                                     arrow::MemoryPool* pool) {
  impl_.reset(new RecordBatchStreamImpl(reader, pool));
}

RecordBatchStream::~RecordBatchStream() {}

std::shared_ptr<arrow::Schema> RecordBatchStream::schema() { return impl_->schema(); }

arrow::Status RecordBatchStream::GetSchemaPayload(FlightPayload* payload) {
  return impl_->GetSchemaPayload(payload);
}

arrow::Status RecordBatchStream::Next(FlightPayload* payload) { return impl_->Next(payload); }

TableRecordBatchStream::TableRecordBatchStream(std::shared_ptr<arrow::TableBatchReader> reader,
                                               std::vector<std::shared_ptr<CachedColumn>> columns,
                                               std::shared_ptr<arrow::Table> table) :
                                               RecordBatchStream(reader), columns_(columns), table_(table) {
}

// ----------------------------------------------------------------------
// Implement FileBatchStream

class FileBatchStream::FileBatchStreamImpl {
 public:
  FileBatchStreamImpl(const std::shared_ptr<FileBatchReader>& reader,
                      std::vector<std::shared_ptr<CachedColumn>> columns,
  										arrow::MemoryPool* pool)
      : pool_(pool), reader_(reader), columns_(columns),
        ipc_options_(arrow::ipc::IpcOptions::Defaults()) {}

  std::shared_ptr<arrow::Schema> schema() { return reader_->schema(); }

  arrow::Status GetSchemaPayload(FlightPayload* payload) {
    return arrow::ipc::internal::GetSchemaPayload(*reader_->schema(), ipc_options_,
                                           &dictionary_memo_, &payload->ipc_message);
  }

  arrow::Status Next(FlightPayload* payload) {
    
    RETURN_NOT_OK(reader_->ReadNext(&current_batch_));

    // TODO(wesm): Delta dictionaries
    if (!current_batch_) {
      // Signal that iteration is over
      payload->ipc_message.metadata = nullptr;
      return arrow::Status::OK();
    } else {
      return internal::GetFileBatchPayload(*current_batch_, ipc_options_,
                                                  &payload->ipc_message);
    }
  }

 private:
  std::shared_ptr<FileBatchReader> reader_;
  std::vector<std::shared_ptr<CachedColumn>> columns_;
  arrow::MemoryPool* pool_;
  arrow::ipc::DictionaryMemo dictionary_memo_;
  arrow::ipc::IpcOptions ipc_options_;
  std::shared_ptr<FileBatch> current_batch_;
};

FileBatchStream::FileBatchStream(const std::shared_ptr<FileBatchReader>& reader,
                                 std::vector<std::shared_ptr<CachedColumn>> columns,
                                 arrow::MemoryPool* pool) {
  impl_.reset(new FileBatchStreamImpl(reader, columns, pool));
}

FileBatchStream::~FileBatchStream() {}

std::shared_ptr<arrow::Schema> FileBatchStream::schema() { return impl_->schema(); }

arrow::Status FileBatchStream::GetSchemaPayload(FlightPayload* payload) {
  return impl_->GetSchemaPayload(payload);
}

arrow::Status FileBatchStream::Next(FlightPayload* payload) { return impl_->Next(payload); }

}  // namespace rpc
}  // namespace pegasus
