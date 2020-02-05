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

#include <cstdint>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "arrow/status.h"
#include "arrow/testing/util.h"

#include "test/gtest-util.h"
#include "rpc/client_auth.h"
#include "rpc/server.h"
#include "rpc/server_auth.h"
#include "rpc/types.h"
#include "rpc/visibility.h"

namespace boost {
namespace process {

class child;

}  // namespace process
}  // namespace boost

namespace pegasus {
namespace rpc {

using Status = arrow::Status;

// ----------------------------------------------------------------------
// Fixture to use for running test servers

class PEGASUS_RPC_EXPORT TestServer {
 public:
  explicit TestServer(const std::string& executable_name)
      : executable_name_(executable_name), port_(::arrow::GetListenPort()) {}
  explicit TestServer(const std::string& executable_name, int port)
      : executable_name_(executable_name), port_(port) {}

  void Start();

  int Stop();

  bool IsRunning();

  int port() const;

 private:
  std::string executable_name_;
  int port_;
  std::shared_ptr<::boost::process::child> server_process_;
};

/// \brief Create a simple Flight server for testing
PEGASUS_RPC_EXPORT
std::unique_ptr<FlightServerBase> ExampleTestServer();

// ----------------------------------------------------------------------
// A RecordBatchReader for serving a sequence of in-memory record batches

// Silence warning
// "non dll-interface class RecordBatchReader used as base for dll-interface class"
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4275)
#endif

class PEGASUS_RPC_EXPORT BatchIterator : public arrow::RecordBatchReader {
 public:
  BatchIterator(const std::shared_ptr<arrow::Schema>& schema,
                const std::vector<std::shared_ptr<arrow::RecordBatch>>& batches)
      : schema_(schema), batches_(batches), position_(0) {}

  std::shared_ptr<arrow::Schema> schema() const override { return schema_; }

  arrow::Status ReadNext(std::shared_ptr<arrow::RecordBatch>* out) override {
    if (position_ >= batches_.size()) {
      *out = nullptr;
    } else {
      *out = batches_[position_++];
    }
    return arrow::Status::OK();
  }

 private:
  std::shared_ptr<arrow::Schema> schema_;
  std::vector<std::shared_ptr<arrow::RecordBatch>> batches_;
  size_t position_;
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

// ----------------------------------------------------------------------
// A FlightDataStream that numbers the record batches
/// \brief A basic implementation of FlightDataStream that will provide
/// a sequence of FlightData messages to be written to a gRPC stream
class PEGASUS_RPC_EXPORT NumberingStream : public FlightDataStream {
 public:
  explicit NumberingStream(std::unique_ptr<FlightDataStream> stream);

  std::shared_ptr<arrow::Schema> schema() override;
  arrow::Status GetSchemaPayload(FlightPayload* payload) override;
  arrow::Status Next(FlightPayload* payload) override;

 private:
  int counter_;
  std::shared_ptr<FlightDataStream> stream_;
};

// ----------------------------------------------------------------------
// Example data for test-server and unit tests

using BatchVector = std::vector<std::shared_ptr<arrow::RecordBatch>>;

PEGASUS_RPC_EXPORT
std::shared_ptr<arrow::Schema> ExampleIntSchema();

PEGASUS_RPC_EXPORT
std::shared_ptr<arrow::Schema> ExampleStringSchema();

PEGASUS_RPC_EXPORT
std::shared_ptr<arrow::Schema> ExampleDictSchema();

PEGASUS_RPC_EXPORT
arrow::Status ExampleIntBatches(BatchVector* out);

PEGASUS_RPC_EXPORT
arrow::Status ExampleDictBatches(BatchVector* out);

PEGASUS_RPC_EXPORT
std::vector<FlightInfo> ExampleFlightInfo();

PEGASUS_RPC_EXPORT
std::vector<ActionType> ExampleActionTypes();

PEGASUS_RPC_EXPORT
arrow::Status MakeFlightInfo(const arrow::Schema& schema, const FlightDescriptor& descriptor,
                      const std::vector<FlightEndpoint>& endpoints, int64_t total_records,
                      int64_t total_bytes, FlightInfo::Data* out);

// ----------------------------------------------------------------------
// A pair of authentication handlers that check for a predefined password
// and set the peer identity to a predefined username.

class PEGASUS_RPC_EXPORT TestServerAuthHandler : public ServerAuthHandler {
 public:
  explicit TestServerAuthHandler(const std::string& username,
                                 const std::string& password);
  ~TestServerAuthHandler() override;
  arrow::Status Authenticate(ServerAuthSender* outgoing, ServerAuthReader* incoming) override;
  arrow::Status IsValid(const std::string& token, std::string* peer_identity) override;

 private:
  std::string username_;
  std::string password_;
};

class PEGASUS_RPC_EXPORT TestServerBasicAuthHandler : public ServerAuthHandler {
 public:
  explicit TestServerBasicAuthHandler(const std::string& username,
                                      const std::string& password);
  ~TestServerBasicAuthHandler() override;
  arrow::Status Authenticate(ServerAuthSender* outgoing, ServerAuthReader* incoming) override;
  arrow::Status IsValid(const std::string& token, std::string* peer_identity) override;

 private:
  BasicAuth basic_auth_;
};

class PEGASUS_RPC_EXPORT TestClientAuthHandler : public ClientAuthHandler {
 public:
  explicit TestClientAuthHandler(const std::string& username,
                                 const std::string& password);
  ~TestClientAuthHandler() override;
  arrow::Status Authenticate(ClientAuthSender* outgoing, ClientAuthReader* incoming) override;
  arrow::Status GetToken(std::string* token) override;

 private:
  std::string username_;
  std::string password_;
};

class PEGASUS_RPC_EXPORT TestClientBasicAuthHandler : public ClientAuthHandler {
 public:
  explicit TestClientBasicAuthHandler(const std::string& username,
                                      const std::string& password);
  ~TestClientBasicAuthHandler() override;
  arrow::Status Authenticate(ClientAuthSender* outgoing, ClientAuthReader* incoming) override;
  arrow::Status GetToken(std::string* token) override;

 private:
  BasicAuth basic_auth_;
  std::string token_;
};

PEGASUS_RPC_EXPORT
Status ExampleTlsCertificates(std::vector<CertKeyPair>* out);

PEGASUS_RPC_EXPORT
Status ExampleTlsCertificateRoot(CertKeyPair* out);

}  // namespace rpc
}  // namespace pegasus
