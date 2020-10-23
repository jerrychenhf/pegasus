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
#include <mutex>
#include <sstream>
#include <string>
#include <vector>
#include <gflags/gflags.h>
#include "arrow/api.h"
#include "arrow/io/memory.h"
#include "arrow/ipc/api.h"
#include "arrow/record_batch.h"
#include "arrow/testing/gtest_util.h"
#include "arrow/util/stopwatch.h"
#include "arrow/util/thread_pool.h"
#include "rpc/api.h"
#include "rpc/test_util.h"

DEFINE_string(server_host, "",
              "An existing performance server to benchmark against (leave blank to spawn "
              "one automatically)");
DEFINE_int32(server_port, 30001, "The port to connect to");
DEFINE_int32(num_threads, 10, "Number of concurrent gets");
DEFINE_int32(records_per_batch, 4096, "Total records per batch within stream");
DEFINE_bool(test_put, false, "Test DoPut instead of DoGet");

namespace pegasus {

using arrow::internal::StopWatch;
using arrow::internal::ThreadPool;

namespace benchmark {

struct PerformanceResult {
  int64_t num_records;
  int64_t num_bytes;
};

struct PerformanceStats {
  PerformanceStats() : total_records(0), total_bytes(0) {}
  std::mutex mutex;
  int64_t total_records;
  int64_t total_bytes;

  void Update(const int64_t total_records, const int64_t total_bytes) {
    std::lock_guard<std::mutex> lock(this->mutex);
    this->total_records += total_records;
    this->total_bytes += total_bytes;
  }
};

arrow::Result<PerformanceResult> RunDoGetTest(rpc::FlightClient* client,
                                              const rpc::FlightEndpoint& endpoint) {
  std::unique_ptr<rpc::FlightStreamReader> reader;
  RETURN_NOT_OK(client->DoGet(endpoint.ticket, &reader));

  rpc::FlightStreamChunk batch;

  // This is hard-coded for right now
  const int bytes_per_record = 165;

  int64_t num_bytes = 0;
  int64_t num_records = 0;
  while (true) {
    RETURN_NOT_OK(reader->Next(&batch));
    if (!batch.data) {
      break;
    }

    num_records += batch.data->num_rows();
    // Hard-coded
    num_bytes += batch.data->num_rows() * bytes_per_record;
  }
  return PerformanceResult{num_records, num_bytes};
}

arrow::Status RunPerformanceTest(rpc::FlightClient* client, bool test_put) {
  // Plan the query
  std::string path = "hdfs://10.239.47.55:9000/genData101/store_sales";

  std::unordered_map<std::string, std::string> options;
  options.insert(std::make_pair("table.location", path));
  options.insert(std::make_pair("catalog.provider", "SPARK"));
  options.insert(std::make_pair("file.format", "PARQUET"));

  rpc::FlightDescriptor descriptor{rpc::FlightDescriptor::PATH, "", {path}, options};

  std::unique_ptr<rpc::FlightInfo> plan;
  RETURN_NOT_OK(client->GetFlightInfo(descriptor, &plan));

  PerformanceStats stats;
  auto test_loop = &RunDoGetTest;
  auto ConsumeStream = [&stats, &test_loop](const rpc::FlightEndpoint& endpoint) {

    std::unique_ptr<rpc::FlightClient> client;
    RETURN_NOT_OK(rpc::FlightClient::Connect(endpoint.locations.front(), &client));

    const auto& result = test_loop(client.get(), endpoint);
    if (result.ok()) {
      const PerformanceResult& perf = result.ValueOrDie();
      stats.Update(perf.num_records, perf.num_bytes);
    }
    return result.status();
  };

  StopWatch timer;
  timer.Start();

  ARROW_ASSIGN_OR_RAISE(auto pool, ThreadPool::Make(FLAGS_num_threads));
  std::vector<arrow::Future<arrow::Status>> tasks;
  std::vector<rpc::FlightEndpoint> endpoints = plan->endpoints();

  for (int i = 0; i < FLAGS_num_threads; i++) {
    ARROW_ASSIGN_OR_RAISE(auto task, pool->Submit(ConsumeStream, endpoints[0]));
    tasks.push_back(std::move(task));
  }

  // Wait for tasks to finish
  for (auto&& task : tasks) {
    RETURN_NOT_OK(task.status());
  }

  // Elapsed time in seconds
  uint64_t elapsed_nanos = timer.Stop();
  double time_elapsed =
      static_cast<double>(elapsed_nanos) / static_cast<double>(1000000000);

  constexpr double kMegabyte = static_cast<double>(1 << 20);

  if (FLAGS_test_put) {
    std::cout << "Bytes written: " << stats.total_bytes << std::endl;
  } else {
    std::cout << "Bytes read: " << stats.total_bytes << std::endl;
  }

  std::cout << "Nanos: " << elapsed_nanos << std::endl;
  std::cout << "Speed: "
            << (static_cast<double>(stats.total_bytes) / kMegabyte / time_elapsed)
            << " MB/s" << std::endl;
  return arrow::Status::OK();
}

}  // namespace benchmark
}  // namespace pegasus

int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  std::string hostname = "localhost";
  if (FLAGS_server_host == "") {
    std::cout << "Using standalone server: localhost" << std::endl;

  } else {
    std::cout << "Using standalone server: " << FLAGS_server_host << std::endl;
    hostname = FLAGS_server_host;
  }

  std::cout << "Testing method: ";
  std::cout << "DoGet";
  std::cout << std::endl;

  std::cout << "Server host: " << hostname << std::endl
            << "Server port: " << FLAGS_server_port << std::endl;

  std::unique_ptr<pegasus::rpc::FlightClient> client;
  pegasus::rpc::Location location;
  ABORT_NOT_OK(
      pegasus::rpc::Location::ForGrpcTcp(hostname, FLAGS_server_port, &location));
  ABORT_NOT_OK(pegasus::rpc::FlightClient::Connect(location, &client));

  arrow::Status s = pegasus::benchmark::RunPerformanceTest(client.get(), FLAGS_test_put);

  if (!s.ok()) {
    std::cerr << "Failed with error: << " << s.ToString() << std::endl;
  }

  return 0;
}
