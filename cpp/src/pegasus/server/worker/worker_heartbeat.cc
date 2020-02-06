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

#include "server/worker/worker_heartbeat.h"

#include <string>
#include "gutil/strings/substitute.h"
#include "util/global_flags.h"
#include "util/time.h"
#include "util/logging.h"
#include "util/thread-pool.h"
#include "runtime/client_cache.h"
#include "rpc/client.h"
#include "rpc/types.h"

DECLARE_string(planner_hostname);
DECLARE_int32(planner_port);

DEFINE_int32(worker_heartbeat_frequency_ms, 1000, "(Advanced) Frequency (in ms) with"
    " which the worker sends heartbeat to planner.");

// Heartbeats that miss their deadline by this much are logged.
const uint32_t DEADLINE_MISS_THRESHOLD_MS = 2000;

namespace pegasus {

typedef ClientConnection<rpc::FlightClient> FlightClientConnection;

WorkerHeartbeat::WorkerHeartbeat()
  : heartbeat_client_cache_(new FlightClientCache())
{
  heartbeat_threadpool_ = std::unique_ptr<ThreadPool<ScheduledHeartbeat>>(
    new ThreadPool<ScheduledHeartbeat>("worker-heartbeat",
        "worker-heartbeat",
        1,
        1,
        bind<void>(mem_fn(&WorkerHeartbeat::DoHeartbeat), this,
          _1, _2)));
}

WorkerHeartbeat::~WorkerHeartbeat() {
  
}

Status WorkerHeartbeat::Init() {
  RETURN_IF_ERROR(heartbeat_threadpool_->Init());

  // Offer with an immediate schedule.
  ScheduledHeartbeat heartbeat(0);
  RETURN_IF_ERROR(OfferHeartbeat(heartbeat));

  return Status::OK();
}

Status WorkerHeartbeat::Start() {
  //TO DO INFO LOG
  //std::cout << "Worker listening on:" << FLAGS_worker_hostname << ":" << FLAGS_worker_port << std::endl;
  
  return Status::OK();
}

Status WorkerHeartbeat::Stop() {
  heartbeat_threadpool_->Shutdown();
  heartbeat_threadpool_->Join();
  return Status::OK();
}

Status WorkerHeartbeat::OfferHeartbeat(const ScheduledHeartbeat& heartbeat) {
  if (!heartbeat_threadpool_->Offer(heartbeat)) {
    stringstream ss;
    ss << "Failed to schedule heartbeat task to thread pool.";
    LOG(ERROR) << ss.str();
    return Status::UnknownError(ss.str());
  }

  return Status::OK();
}

void WorkerHeartbeat::DoHeartbeat(int thread_id,
    const ScheduledHeartbeat& heartbeat) {
  int64_t heartbeat_deadline = heartbeat.deadline;
  if (heartbeat_deadline != 0) {
    // Wait until deadline.
    int64_t diff_ms = heartbeat_deadline - UnixMillis();
    while (diff_ms > 0) {
      SleepForMs(diff_ms);
      diff_ms = heartbeat_deadline - UnixMillis();
    }
    
    diff_ms = std::abs(diff_ms);
    LOG(INFO) << "Sending heartbeat message to: "
      << " (deadline accuracy: " << diff_ms << "ms)";

    if (diff_ms > DEADLINE_MISS_THRESHOLD_MS) {
      const string& msg = strings::Substitute(
          "Missed heartbeat deadline by $0ms, "
          "consider increasing --worker_heartbeat_frequency_ms (currently $1)",
          diff_ms,
          FLAGS_worker_heartbeat_frequency_ms);
      LOG(WARNING) << msg;
    }
    // Don't warn for topic updates - they can be slow and still correct. Recommending an
    // increase in update period will just confuse (as will increasing the thread pool
    // size) because it's hard for users to pick a good value, they may still see these
    // messages and it won't be a correctness problem.
  } else {
    // The first update is scheduled immediately and has a deadline of 0. There's no need
    // to wait.
    LOG(INFO) << "Initial heartbeat message.";
  }
  
  // Send the heartbeat message, and compute the next deadline
  int64_t deadline_ms = 0;
  Status status = SendHeartbeat(heartbeat);
  if (status.ok()) {
    //refresh the last heartbeat timestamp
  } else if (status.code() == StatusCode::RpcTimeout) {
    // Add details to status to make it more useful, while preserving the stack
  }

  deadline_ms = UnixMillis() + FLAGS_worker_heartbeat_frequency_ms;
  
  // Schedule the next message.
  LOG(INFO) << "Next heartbeat deadlineis in " << deadline_ms << "ms";
  status = OfferHeartbeat(ScheduledHeartbeat(deadline_ms));
  if (!status.ok()) {
    LOG(WARNING) << "Unable to send next heartbeat message: "
                  << status.message();
  }
}

Status WorkerHeartbeat::SendHeartbeat(const ScheduledHeartbeat& heartbeat) {
  Status status;
  std::string planner_address = FLAGS_planner_hostname + ":" 
    + std::to_string(FLAGS_planner_port);
  FlightClientConnection client(heartbeat_client_cache_.get(),
      planner_address, &status);
  RETURN_IF_ERROR(status);

  rpc::HeartbeatInfo info;
  std::unique_ptr<rpc::HeartbeatResult> result;
  arrow::Status arrowStatus = client->Heartbeat(info, &result);
  status = Status::fromArrowStatus(arrowStatus);
  RETURN_IF_ERROR(status);
      
  if(heartbeat.heartbeatType == HeartbeatType::REGISTRATION) {
    registered_ = true;
  }
  
  return Status::OK();
}

} // namespace pegasus