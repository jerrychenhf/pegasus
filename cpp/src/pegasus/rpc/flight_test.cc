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

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "arrow/ipc/test_common.h"
#include "arrow/status.h"
#include "arrow/testing/gtest_util.h"
#include "arrow/testing/util.h"
#include "arrow/util/make_unique.h"

#include "pegasus/rpc/api.h"

#ifdef GRPCPP_GRPCPP_H
#error "gRPC headers should not be in public API"
#endif

#include "pegasus/rpc/internal.h"
#include "pegasus/rpc/middleware_internal.h"
#include "pegasus/rpc/test_util.h"

namespace pb = pegasus::rpc::protocol;

namespace pegasus {
namespace rpc {

void AssertEqual(const ActionType& expected, const ActionType& actual) {
  ASSERT_EQ(expected.type, actual.type);
  ASSERT_EQ(expected.description, actual.description);
}

void AssertEqual(const FlightDescriptor& expected, const FlightDescriptor& actual) {
  ASSERT_TRUE(expected.Equals(actual));
}

void AssertEqual(const Ticket& expected, const Ticket& actual) {
  ASSERT_EQ(expected.ticket, actual.ticket);
}

void AssertEqual(const Location& expected, const Location& actual) {
  ASSERT_EQ(expected, actual);
}

void AssertEqual(const std::vector<FlightEndpoint>& expected,
                 const std::vector<FlightEndpoint>& actual) {
  ASSERT_EQ(expected.size(), actual.size());
  for (size_t i = 0; i < expected.size(); ++i) {
    AssertEqual(expected[i].ticket, actual[i].ticket);

    ASSERT_EQ(expected[i].locations.size(), actual[i].locations.size());
    for (size_t j = 0; j < expected[i].locations.size(); ++j) {
      AssertEqual(expected[i].locations[j], actual[i].locations[j]);
    }
  }
}

template <typename T>
void AssertEqual(const std::vector<T>& expected, const std::vector<T>& actual) {
  ASSERT_EQ(expected.size(), actual.size());
  for (size_t i = 0; i < expected.size(); ++i) {
    AssertEqual(expected[i], actual[i]);
  }
}

void AssertEqual(const FlightInfo& expected, const FlightInfo& actual) {
  std::shared_ptr<arrow::Schema> ex_schema, actual_schema;
  arrow::ipc::DictionaryMemo expected_memo;
  arrow::ipc::DictionaryMemo actual_memo;
  ASSERT_OK(expected.GetSchema(&expected_memo, &ex_schema));
  ASSERT_OK(actual.GetSchema(&actual_memo, &actual_schema));

  AssertSchemaEqual(*ex_schema, *actual_schema);
  ASSERT_EQ(expected.total_records(), actual.total_records());
  ASSERT_EQ(expected.total_bytes(), actual.total_bytes());

  AssertEqual(expected.descriptor(), actual.descriptor());
  AssertEqual(expected.endpoints(), actual.endpoints());
}

TEST(TestFlightDescriptor, Basics) {
  auto a = FlightDescriptor::Command("select * from table");
  auto b = FlightDescriptor::Command("select * from table");
  auto c = FlightDescriptor::Command("select foo from table");
  auto d = FlightDescriptor::Path({"foo", "bar"});
  auto e = FlightDescriptor::Path({"foo", "baz"});
  auto f = FlightDescriptor::Path({"foo", "baz"});

  ASSERT_EQ(a.ToString(), "FlightDescriptor<cmd = 'select * from table'>");
  ASSERT_EQ(d.ToString(), "FlightDescriptor<path = 'foo/bar'>");
  ASSERT_TRUE(a.Equals(b));
  ASSERT_FALSE(a.Equals(c));
  ASSERT_FALSE(a.Equals(d));
  ASSERT_FALSE(d.Equals(e));
  ASSERT_TRUE(e.Equals(f));
}

// This tests the internal protobuf types which don't get exported in the Flight DLL.
#ifndef _WIN32
TEST(TestFlightDescriptor, ToFromProto) {
  FlightDescriptor descr_test;
  pb::FlightDescriptor pb_descr;

  FlightDescriptor descr1{FlightDescriptor::PATH, "", {"foo", "bar"}};
  ASSERT_OK(internal::ToProto(descr1, &pb_descr));
  ASSERT_OK(internal::FromProto(pb_descr, &descr_test));
  AssertEqual(descr1, descr_test);

  FlightDescriptor descr2{FlightDescriptor::CMD, "command", {}};
  ASSERT_OK(internal::ToProto(descr2, &pb_descr));
  ASSERT_OK(internal::FromProto(pb_descr, &descr_test));
  AssertEqual(descr2, descr_test);
}
#endif

TEST(TestFlight, DISABLED_StartStopTestServer) {
  TestServer server("flight-test-server");
  server.Start();
  ASSERT_TRUE(server.IsRunning());

  std::this_thread::sleep_for(std::chrono::duration<double>(0.2));

  ASSERT_TRUE(server.IsRunning());
  int exit_code = server.Stop();
#ifdef _WIN32
  // We do a hard kill on Windows
  ASSERT_EQ(259, exit_code);
#else
  ASSERT_EQ(0, exit_code);
#endif
  ASSERT_FALSE(server.IsRunning());
}

// ARROW-6017: we should be able to construct locations for unknown
// schemes
TEST(TestFlight, UnknownLocationScheme) {
  Location location;
  ASSERT_OK(Location::Parse("s3://test", &location));
  ASSERT_OK(Location::Parse("https://example.com/foo", &location));
}

TEST(TestFlight, ConnectUri) {
  TestServer server("flight-test-server");
  server.Start();
  ASSERT_TRUE(server.IsRunning());

  std::stringstream ss;
  ss << "grpc://localhost:" << server.port();
  std::string uri = ss.str();

  std::unique_ptr<FlightClient> client;
  Location location1;
  Location location2;
  ASSERT_OK(Location::Parse(uri, &location1));
  ASSERT_OK(Location::Parse(uri, &location2));
  ASSERT_OK(FlightClient::Connect(location1, &client));
  ASSERT_OK(FlightClient::Connect(location2, &client));
}

TEST(TestFlight, RoundTripTypes) {
  Ticket ticket{"foo"};
  std::string ticket_serialized;
  Ticket ticket_deserialized;
  ASSERT_OK(ticket.SerializeToString(&ticket_serialized));
  ASSERT_OK(Ticket::Deserialize(ticket_serialized, &ticket_deserialized));
  ASSERT_EQ(ticket.ticket, ticket_deserialized.ticket);

  FlightDescriptor desc = FlightDescriptor::Command("select * from foo;");
  std::string desc_serialized;
  FlightDescriptor desc_deserialized;
  ASSERT_OK(desc.SerializeToString(&desc_serialized));
  ASSERT_OK(FlightDescriptor::Deserialize(desc_serialized, &desc_deserialized));
  ASSERT_TRUE(desc.Equals(desc_deserialized));

  desc = FlightDescriptor::Path({"a", "b", "test.arrow"});
  ASSERT_OK(desc.SerializeToString(&desc_serialized));
  ASSERT_OK(FlightDescriptor::Deserialize(desc_serialized, &desc_deserialized));
  ASSERT_TRUE(desc.Equals(desc_deserialized));

  FlightInfo::Data data;
  std::shared_ptr<arrow::Schema> schema =
      arrow::schema({field("a", arrow::int64()), field("b", arrow::int64()), field("c", arrow::int64()),
                     field("d", arrow::int64())});
  Location location1, location2, location3;
  ASSERT_OK(Location::ForGrpcTcp("localhost", 10010, &location1));
  ASSERT_OK(Location::ForGrpcTls("localhost", 10010, &location2));
  ASSERT_OK(Location::ForGrpcUnix("/tmp/test.sock", &location3));
  std::vector<FlightEndpoint> endpoints{FlightEndpoint{ticket, {location1, location2}},
                                        FlightEndpoint{ticket, {location3}}};
  ASSERT_OK(MakeFlightInfo(*schema, desc, endpoints, -1, -1, &data));
  std::unique_ptr<FlightInfo> info = std::unique_ptr<FlightInfo>(new FlightInfo(data));
  std::string info_serialized;
  std::unique_ptr<FlightInfo> info_deserialized;
  ASSERT_OK(info->SerializeToString(&info_serialized));
  ASSERT_OK(FlightInfo::Deserialize(info_serialized, &info_deserialized));
  ASSERT_TRUE(info->descriptor().Equals(info_deserialized->descriptor()));
  ASSERT_EQ(info->endpoints(), info_deserialized->endpoints());
  ASSERT_EQ(info->total_records(), info_deserialized->total_records());
  ASSERT_EQ(info->total_bytes(), info_deserialized->total_bytes());
}

TEST(TestFlight, RoundtripStatus) {
  // Make sure status codes round trip through our conversions

  std::shared_ptr<FlightStatusDetail> detail;
  detail = FlightStatusDetail::UnwrapStatus(
      MakeFlightError(FlightStatusCode::Internal, "Test message"));
  ASSERT_NE(nullptr, detail);
  ASSERT_EQ(FlightStatusCode::Internal, detail->code());

  detail = FlightStatusDetail::UnwrapStatus(
      MakeFlightError(FlightStatusCode::TimedOut, "Test message"));
  ASSERT_NE(nullptr, detail);
  ASSERT_EQ(FlightStatusCode::TimedOut, detail->code());

  detail = FlightStatusDetail::UnwrapStatus(
      MakeFlightError(FlightStatusCode::Cancelled, "Test message"));
  ASSERT_NE(nullptr, detail);
  ASSERT_EQ(FlightStatusCode::Cancelled, detail->code());

  detail = FlightStatusDetail::UnwrapStatus(
      MakeFlightError(FlightStatusCode::Unauthenticated, "Test message"));
  ASSERT_NE(nullptr, detail);
  ASSERT_EQ(FlightStatusCode::Unauthenticated, detail->code());

  detail = FlightStatusDetail::UnwrapStatus(
      MakeFlightError(FlightStatusCode::Unauthorized, "Test message"));
  ASSERT_NE(nullptr, detail);
  ASSERT_EQ(FlightStatusCode::Unauthorized, detail->code());

  detail = FlightStatusDetail::UnwrapStatus(
      MakeFlightError(FlightStatusCode::Unavailable, "Test message"));
  ASSERT_NE(nullptr, detail);
  ASSERT_EQ(FlightStatusCode::Unavailable, detail->code());
}

TEST(TestFlight, GetPort) {
  Location location;
  std::unique_ptr<FlightServerBase> server = ExampleTestServer();

  ASSERT_OK(Location::ForGrpcTcp("localhost", 0, &location));
  FlightServerOptions options(location);
  ASSERT_OK(server->Init(options));
  ASSERT_GT(server->port(), 0);
}

TEST(TestFlight, BuilderHook) {
  Location location;
  std::unique_ptr<FlightServerBase> server = ExampleTestServer();

  ASSERT_OK(Location::ForGrpcTcp("localhost", 0, &location));
  FlightServerOptions options(location);
  bool builder_hook_run = false;
  options.builder_hook = [&builder_hook_run](void* builder) {
    ASSERT_NE(nullptr, builder);
    builder_hook_run = true;
  };
  ASSERT_OK(server->Init(options));
  ASSERT_TRUE(builder_hook_run);
  ASSERT_GT(server->port(), 0);
  ASSERT_OK(server->Shutdown());
}

// ----------------------------------------------------------------------
// Client tests

// Helper to initialize a server and matching client with callbacks to
// populate options.
template <typename T, typename... Args>
Status MakeServer(std::unique_ptr<FlightServerBase>* server,
                  std::unique_ptr<FlightClient>* client,
                  std::function<Status(FlightServerOptions*)> make_server_options,
                  std::function<Status(FlightClientOptions*)> make_client_options,
                  Args&&... server_args) {
  Location location;
  RETURN_NOT_OK(Location::ForGrpcTcp("localhost", 0, &location));
  *server = arrow::internal::make_unique<T>(std::forward<Args>(server_args)...);
  FlightServerOptions server_options(location);
  RETURN_NOT_OK(make_server_options(&server_options));
  RETURN_NOT_OK((*server)->Init(server_options));
  Location real_location;
  RETURN_NOT_OK(Location::ForGrpcTcp("localhost", (*server)->port(), &real_location));
  FlightClientOptions client_options;
  RETURN_NOT_OK(make_client_options(&client_options));
  return FlightClient::Connect(real_location, client_options, client);
}

class TestFlightClient : public ::testing::Test {
 public:
  void SetUp() {
    server_ = ExampleTestServer();

    Location location;
    ASSERT_OK(Location::ForGrpcTcp("localhost", 0, &location));
    FlightServerOptions options(location);
    ASSERT_OK(server_->Init(options));

    ASSERT_OK(ConnectClient());
  }

  void TearDown() { ASSERT_OK(server_->Shutdown()); }

  Status ConnectClient() {
    Location location;
    RETURN_NOT_OK(Location::ForGrpcTcp("localhost", server_->port(), &location));
    return FlightClient::Connect(location, &client_);
  }

  template <typename EndpointCheckFunc>
  void CheckDoGet(const FlightDescriptor& descr, const BatchVector& expected_batches,
                  EndpointCheckFunc&& check_endpoints) {
    auto num_batches = static_cast<int>(expected_batches.size());
    ASSERT_GE(num_batches, 2);
    auto expected_schema = expected_batches[0]->schema();

    std::unique_ptr<FlightInfo> info;
    ASSERT_OK(client_->GetFlightInfo(descr, &info));
    check_endpoints(info->endpoints());

    std::shared_ptr<arrow::Schema> schema;
    arrow::ipc::DictionaryMemo dict_memo;
    ASSERT_OK(info->GetSchema(&dict_memo, &schema));
    AssertSchemaEqual(*expected_schema, *schema);

    // By convention, fetch the first endpoint
    Ticket ticket = info->endpoints()[0].ticket;
    std::unique_ptr<FlightStreamReader> stream;
    ASSERT_OK(client_->DoGet(ticket, &stream));

    FlightStreamChunk chunk;
    for (int i = 0; i < num_batches; ++i) {
      ASSERT_OK(stream->Next(&chunk));
      ASSERT_NE(nullptr, chunk.data);
      ASSERT_BATCHES_EQUAL(*expected_batches[i], *chunk.data);
    }

    // Stream exhausted
    ASSERT_OK(stream->Next(&chunk));
    ASSERT_EQ(nullptr, chunk.data);
  }

 protected:
  std::unique_ptr<FlightClient> client_;
  std::unique_ptr<FlightServerBase> server_;
};

class AuthTestServer : public FlightServerBase {
  Status DoAction(const ServerCallContext& context, const Action& action,
                  std::unique_ptr<ResultStream>* result) override {
    std::shared_ptr<arrow::Buffer> buf;
    RETURN_NOT_OK(arrow::Buffer::FromString(context.peer_identity(), &buf));
    *result = std::unique_ptr<ResultStream>(new SimpleResultStream({Result{buf}}));
    return Status::OK();
  }
};

class TlsTestServer : public FlightServerBase {
  Status DoAction(const ServerCallContext& context, const Action& action,
                  std::unique_ptr<ResultStream>* result) override {
    std::shared_ptr<arrow::Buffer> buf;
    RETURN_NOT_OK(arrow::Buffer::FromString("Hello, world!", &buf));
    *result = std::unique_ptr<ResultStream>(new SimpleResultStream({Result{buf}}));
    return Status::OK();
  }
};

class DoPutTestServer : public FlightServerBase {
 public:
  Status DoPut(const ServerCallContext& context,
               std::unique_ptr<FlightMessageReader> reader,
               std::unique_ptr<FlightMetadataWriter> writer) override {
    descriptor_ = reader->descriptor();
    return reader->ReadAll(&batches_);
  }

 protected:
  FlightDescriptor descriptor_;
  BatchVector batches_;

  friend class TestDoPut;
};

class MetadataTestServer : public FlightServerBase {
  Status DoGet(const ServerCallContext& context, const Ticket& request,
               std::unique_ptr<FlightDataStream>* data_stream) override {
    BatchVector batches;
    RETURN_NOT_OK(ExampleIntBatches(&batches));
    std::shared_ptr<arrow::RecordBatchReader> batch_reader =
        std::make_shared<BatchIterator>(batches[0]->schema(), batches);

    *data_stream = std::unique_ptr<FlightDataStream>(new NumberingStream(
        std::unique_ptr<FlightDataStream>(new RecordBatchStream(batch_reader))));
    return Status::OK();
  }

  Status DoPut(const ServerCallContext& context,
               std::unique_ptr<FlightMessageReader> reader,
               std::unique_ptr<FlightMetadataWriter> writer) override {
    FlightStreamChunk chunk;
    int counter = 0;
    while (true) {
      RETURN_NOT_OK(reader->Next(&chunk));
      if (chunk.data == nullptr) break;
      if (chunk.app_metadata == nullptr) {
        return Status::Invalid("Expected application metadata to be provided");
      }
      if (std::to_string(counter) != chunk.app_metadata->ToString()) {
        return Status::Invalid("Expected metadata value: " + std::to_string(counter) +
                               " but got: " + chunk.app_metadata->ToString());
      }
      auto metadata = arrow::Buffer::FromString(std::to_string(counter));
      RETURN_NOT_OK(writer->WriteMetadata(*metadata));
      counter++;
    }
    return Status::OK();
  }
};

class TestMetadata : public ::testing::Test {
 public:
  void SetUp() {
    ASSERT_OK(MakeServer<MetadataTestServer>(
        &server_, &client_, [](FlightServerOptions* options) { return Status::OK(); },
        [](FlightClientOptions* options) { return Status::OK(); }));
  }

  void TearDown() { ASSERT_OK(server_->Shutdown()); }

 protected:
  std::unique_ptr<FlightClient> client_;
  std::unique_ptr<FlightServerBase> server_;
};

class TestAuthHandler : public ::testing::Test {
 public:
  void SetUp() {
    ASSERT_OK(MakeServer<AuthTestServer>(
        &server_, &client_,
        [](FlightServerOptions* options) {
          options->auth_handler = std::unique_ptr<ServerAuthHandler>(
              new TestServerAuthHandler("user", "p4ssw0rd"));
          return Status::OK();
        },
        [](FlightClientOptions* options) { return Status::OK(); }));
  }

  void TearDown() { ASSERT_OK(server_->Shutdown()); }

 protected:
  std::unique_ptr<FlightClient> client_;
  std::unique_ptr<FlightServerBase> server_;
};

class TestBasicAuthHandler : public ::testing::Test {
 public:
  void SetUp() {
    ASSERT_OK(MakeServer<AuthTestServer>(
        &server_, &client_,
        [](FlightServerOptions* options) {
          options->auth_handler = std::unique_ptr<ServerAuthHandler>(
              new TestServerBasicAuthHandler("user", "p4ssw0rd"));
          return Status::OK();
        },
        [](FlightClientOptions* options) { return Status::OK(); }));
  }

  void TearDown() { ASSERT_OK(server_->Shutdown()); }

 protected:
  std::unique_ptr<FlightClient> client_;
  std::unique_ptr<FlightServerBase> server_;
};

class TestDoPut : public ::testing::Test {
 public:
  void SetUp() {
    ASSERT_OK(MakeServer<DoPutTestServer>(
        &server_, &client_, [](FlightServerOptions* options) { return Status::OK(); },
        [](FlightClientOptions* options) { return Status::OK(); }));
    do_put_server_ = (DoPutTestServer*)server_.get();
  }

  void TearDown() { ASSERT_OK(server_->Shutdown()); }

  void CheckBatches(FlightDescriptor expected_descriptor,
                    const BatchVector& expected_batches) {
    ASSERT_TRUE(do_put_server_->descriptor_.Equals(expected_descriptor));
    ASSERT_EQ(do_put_server_->batches_.size(), expected_batches.size());
    for (size_t i = 0; i < expected_batches.size(); ++i) {
      ASSERT_BATCHES_EQUAL(*do_put_server_->batches_[i], *expected_batches[i]);
    }
  }

  void CheckDoPut(FlightDescriptor descr, const std::shared_ptr<arrow::Schema>& schema,
                  const BatchVector& batches) {
    std::unique_ptr<FlightStreamWriter> stream;
    std::unique_ptr<FlightMetadataReader> reader;
    ASSERT_OK(client_->DoPut(descr, schema, &stream, &reader));
    for (const auto& batch : batches) {
      ASSERT_OK(stream->WriteRecordBatch(*batch));
    }
    ASSERT_OK(stream->DoneWriting());
    ASSERT_OK(stream->Close());

    CheckBatches(descr, batches);
  }

 protected:
  std::unique_ptr<FlightClient> client_;
  std::unique_ptr<FlightServerBase> server_;
  DoPutTestServer* do_put_server_;
};

class TestTls : public ::testing::Test {
 public:
  void SetUp() {
    server_.reset(new TlsTestServer);

    Location location;
    ASSERT_OK(Location::ForGrpcTls("localhost", 0, &location));
    FlightServerOptions options(location);
    ASSERT_RAISES(UnknownError, server_->Init(options));
    ASSERT_OK(ExampleTlsCertificates(&options.tls_certificates));
    ASSERT_OK(server_->Init(options));

    ASSERT_OK(Location::ForGrpcTls("localhost", server_->port(), &location_));
    ASSERT_OK(ConnectClient());
  }

  void TearDown() { ASSERT_OK(server_->Shutdown()); }

  Status ConnectClient() {
    auto options = FlightClientOptions();
    CertKeyPair root_cert;
    RETURN_NOT_OK(ExampleTlsCertificateRoot(&root_cert));
    options.tls_root_certs = root_cert.pem_cert;
    return FlightClient::Connect(location_, options, &client_);
  }

 protected:
  Location location_;
  std::unique_ptr<FlightClient> client_;
  std::unique_ptr<FlightServerBase> server_;
};

// A server middleware that rejects all calls.
class RejectServerMiddlewareFactory : public ServerMiddlewareFactory {
  Status StartCall(const CallInfo& info, const CallHeaders& incoming_headers,
                   std::shared_ptr<ServerMiddleware>* middleware) override {
    return MakeFlightError(FlightStatusCode::Unauthenticated, "All calls are rejected");
  }
};

// A server middleware that counts the number of successful and failed
// calls.
class CountingServerMiddleware : public ServerMiddleware {
 public:
  CountingServerMiddleware(std::atomic<int>* successful, std::atomic<int>* failed)
      : successful_(successful), failed_(failed) {}
  void SendingHeaders(AddCallHeaders* outgoing_headers) override {}
  void CallCompleted(const Status& status) override {
    if (status.ok()) {
      ARROW_IGNORE_EXPR((*successful_)++);
    } else {
      ARROW_IGNORE_EXPR((*failed_)++);
    }
  }

  std::string name() const override { return "CountingServerMiddleware"; }

 private:
  std::atomic<int>* successful_;
  std::atomic<int>* failed_;
};

class CountingServerMiddlewareFactory : public ServerMiddlewareFactory {
 public:
  CountingServerMiddlewareFactory() : successful_(0), failed_(0) {}

  Status StartCall(const CallInfo& info, const CallHeaders& incoming_headers,
                   std::shared_ptr<ServerMiddleware>* middleware) override {
    *middleware = std::make_shared<CountingServerMiddleware>(&successful_, &failed_);
    return Status::OK();
  }

  std::atomic<int> successful_;
  std::atomic<int> failed_;
};

// The current span ID, used to emulate OpenTracing style distributed
// tracing. Only used for communication between application code and
// client middleware.
static thread_local std::string current_span_id = "";

// A server middleware that stores the current span ID, in an
// emulation of OpenTracing style distributed tracing.
class TracingServerMiddleware : public ServerMiddleware {
 public:
  explicit TracingServerMiddleware(const std::string& current_span_id)
      : span_id(current_span_id) {}
  void SendingHeaders(AddCallHeaders* outgoing_headers) override {}
  void CallCompleted(const Status& status) override {}

  std::string name() const override { return "TracingServerMiddleware"; }

  std::string span_id;
};

class TracingServerMiddlewareFactory : public ServerMiddlewareFactory {
 public:
  TracingServerMiddlewareFactory() {}

  Status StartCall(const CallInfo& info, const CallHeaders& incoming_headers,
                   std::shared_ptr<ServerMiddleware>* middleware) override {
    const std::pair<CallHeaders::const_iterator, CallHeaders::const_iterator>& iter_pair =
        incoming_headers.equal_range("x-tracing-span-id");
    if (iter_pair.first != iter_pair.second) {
      const arrow::util::string_view& value = (*iter_pair.first).second;
      *middleware = std::make_shared<TracingServerMiddleware>(std::string(value));
    }
    return Status::OK();
  }
};

// A client middleware that adds a thread-local "request ID" to
// outgoing calls as a header, and keeps track of the status of
// completed calls. NOT thread-safe.
class PropagatingClientMiddleware : public ClientMiddleware {
 public:
  explicit PropagatingClientMiddleware(std::atomic<int>* received_headers,
                                       std::vector<Status>* recorded_status)
      : received_headers_(received_headers), recorded_status_(recorded_status) {}

  void SendingHeaders(AddCallHeaders* outgoing_headers) {
    // Pick up the span ID from thread locals. We have to use a
    // thread-local for communication, since we aren't even
    // instantiated until after the application code has already
    // started the call (and so there's no chance for application code
    // to pass us parameters directly).
    outgoing_headers->AddHeader("x-tracing-span-id", current_span_id);
  }

  void ReceivedHeaders(const CallHeaders& incoming_headers) { (*received_headers_)++; }

  void CallCompleted(const Status& status) { recorded_status_->push_back(status); }

 private:
  std::atomic<int>* received_headers_;
  std::vector<Status>* recorded_status_;
};

class PropagatingClientMiddlewareFactory : public ClientMiddlewareFactory {
 public:
  void StartCall(const CallInfo& info, std::unique_ptr<ClientMiddleware>* middleware) {
    recorded_calls_.push_back(info.method);
    *middleware = arrow::internal::make_unique<PropagatingClientMiddleware>(
        &received_headers_, &recorded_status_);
  }

  void Reset() {
    recorded_calls_.clear();
    recorded_status_.clear();
    received_headers_.fetch_and(0);
  }

  std::vector<FlightMethod> recorded_calls_;
  std::vector<Status> recorded_status_;
  std::atomic<int> received_headers_;
};

class ReportContextTestServer : public FlightServerBase {
  Status DoAction(const ServerCallContext& context, const Action& action,
                  std::unique_ptr<ResultStream>* result) override {
    std::shared_ptr<arrow::Buffer> buf;
    const ServerMiddleware* middleware = context.GetMiddleware("tracing");
    if (middleware == nullptr || middleware->name() != "TracingServerMiddleware") {
      RETURN_NOT_OK(arrow::Buffer::FromString("", &buf));
    } else {
      RETURN_NOT_OK(arrow::Buffer::FromString(
          ((const TracingServerMiddleware*)middleware)->span_id, &buf));
    }
    *result = std::unique_ptr<ResultStream>(new SimpleResultStream({Result{buf}}));
    return Status::OK();
  }
};

class PropagatingTestServer : public FlightServerBase {
 public:
  explicit PropagatingTestServer(std::unique_ptr<FlightClient> client)
      : client_(std::move(client)) {}

  Status DoAction(const ServerCallContext& context, const Action& action,
                  std::unique_ptr<ResultStream>* result) override {
    const ServerMiddleware* middleware = context.GetMiddleware("tracing");
    if (middleware == nullptr || middleware->name() != "TracingServerMiddleware") {
      current_span_id = "";
    } else {
      current_span_id = ((const TracingServerMiddleware*)middleware)->span_id;
    }

    return client_->DoAction(action, result);
  }

 private:
  std::unique_ptr<FlightClient> client_;
};

class TestRejectServerMiddleware : public ::testing::Test {
 public:
  void SetUp() {
    ASSERT_OK(MakeServer<MetadataTestServer>(
        &server_, &client_,
        [](FlightServerOptions* options) {
          options->middleware.push_back(
              {"reject", std::make_shared<RejectServerMiddlewareFactory>()});
          return Status::OK();
        },
        [](FlightClientOptions* options) { return Status::OK(); }));
  }

  void TearDown() { ASSERT_OK(server_->Shutdown()); }

 protected:
  std::unique_ptr<FlightClient> client_;
  std::unique_ptr<FlightServerBase> server_;
};

class TestCountingServerMiddleware : public ::testing::Test {
 public:
  void SetUp() {
    request_counter_ = std::make_shared<CountingServerMiddlewareFactory>();
    ASSERT_OK(MakeServer<MetadataTestServer>(
        &server_, &client_,
        [&](FlightServerOptions* options) {
          options->middleware.push_back({"request_counter", request_counter_});
          return Status::OK();
        },
        [](FlightClientOptions* options) { return Status::OK(); }));
  }

  void TearDown() { ASSERT_OK(server_->Shutdown()); }

 protected:
  std::shared_ptr<CountingServerMiddlewareFactory> request_counter_;
  std::unique_ptr<FlightClient> client_;
  std::unique_ptr<FlightServerBase> server_;
};

// Setup for this test is 2 servers
// 1. Client makes request to server A with a request ID set
// 2. server A extracts the request ID and makes a request to server B
//    with the same request ID set
// 3. server B extracts the request ID and sends it back
// 4. server A returns the response of server B
// 5. Client validates the response
class TestPropagatingMiddleware : public ::testing::Test {
 public:
  void SetUp() {
    server_middleware_ = std::make_shared<TracingServerMiddlewareFactory>();
    second_client_middleware_ = std::make_shared<PropagatingClientMiddlewareFactory>();
    client_middleware_ = std::make_shared<PropagatingClientMiddlewareFactory>();

    std::unique_ptr<FlightClient> server_client;
    ASSERT_OK(MakeServer<ReportContextTestServer>(
        &second_server_, &server_client,
        [&](FlightServerOptions* options) {
          options->middleware.push_back({"tracing", server_middleware_});
          return Status::OK();
        },
        [&](FlightClientOptions* options) {
          options->middleware.push_back(second_client_middleware_);
          return Status::OK();
        }));

    ASSERT_OK(MakeServer<PropagatingTestServer>(
        &first_server_, &client_,
        [&](FlightServerOptions* options) {
          options->middleware.push_back({"tracing", server_middleware_});
          return Status::OK();
        },
        [&](FlightClientOptions* options) {
          options->middleware.push_back(client_middleware_);
          return Status::OK();
        },
        std::move(server_client)));
  }

  void ValidateStatus(const Status& status, const FlightMethod& method) {
    ASSERT_EQ(1, client_middleware_->received_headers_);
    ASSERT_EQ(method, client_middleware_->recorded_calls_.at(0));
    ASSERT_EQ(status.code(), client_middleware_->recorded_status_.at(0).code());
  }

  void TearDown() {
    ASSERT_OK(first_server_->Shutdown());
    ASSERT_OK(second_server_->Shutdown());
  }

  void CheckHeader(const std::string& header, const std::string& value,
                   const CallHeaders::const_iterator& it) {
    // Construct a string_view before comparison to satisfy MSVC
    arrow::util::string_view header_view(header.data(), header.length());
    arrow::util::string_view value_view(value.data(), value.length());
    ASSERT_EQ(header_view, (*it).first);
    ASSERT_EQ(value_view, (*it).second);
  }

 protected:
  std::unique_ptr<FlightClient> client_;
  std::unique_ptr<FlightServerBase> first_server_;
  std::unique_ptr<FlightServerBase> second_server_;
  std::shared_ptr<TracingServerMiddlewareFactory> server_middleware_;
  std::shared_ptr<PropagatingClientMiddlewareFactory> second_client_middleware_;
  std::shared_ptr<PropagatingClientMiddlewareFactory> client_middleware_;
};


}  // namespace rpc
}  // namespace pegasus
