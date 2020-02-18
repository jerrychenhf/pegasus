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

#include "server/planner/worker_failure_detector.h"

#include <string>
#include "gutil/strings/substitute.h"
#include "util/global_flags.h"
#include "util/time.h"
#include "common/logging.h"
#include "util/thread-pool.h"
#include <boost/thread/lock_guard.hpp>
#include "server/planner/worker_manager.h"

using namespace boost;
using boost::get_system_time;
using boost::posix_time::time_duration;
using boost::posix_time::milliseconds;
using boost::system_time;

DECLARE_int32(worker_heartbeat_frequency_ms);
DECLARE_int32(planner_max_missed_heartbeats);    

// Heartbeats that miss their deadline by this much are logged.
const uint32_t DEADLINE_MISS_THRESHOLD_MS = 2000;

namespace pegasus {

WorkerFailureDetector::WorkerFailureDetector(WorkerManager* worker_manager)
: worker_manager_(worker_manager),
  failure_detector_(new MissedHeartbeatFailureDetector(
        FLAGS_planner_max_missed_heartbeats,
        FLAGS_planner_max_missed_heartbeats / 2))
{
  failure_detector_threadpool_ = std::unique_ptr<ThreadPool<ScheduledDetect>>(
    new ThreadPool<ScheduledDetect>("planner-failure-detector",
        "planner-failure-detector",
        1,
        1,
        bind<void>(mem_fn(&WorkerFailureDetector::DoDetect), this,
          _1, _2)));
}

WorkerFailureDetector::~WorkerFailureDetector() {
  
}

Status WorkerFailureDetector::Init() {
  RETURN_IF_ERROR(failure_detector_threadpool_->Init());

  return Status::OK();
}

Status WorkerFailureDetector::Start() {
  LOG(INFO) << "Failure detector for works started.";
  
  // Offer with an immediate schedule.
  ScheduledDetect detect(0);
  RETURN_IF_ERROR(OfferDetect(detect));
  
  return Status::OK();
}

Status WorkerFailureDetector::Stop() {
  failure_detector_threadpool_->Shutdown();
  failure_detector_threadpool_->Join();
  return Status::OK();
}

Status WorkerFailureDetector::OfferDetect(const ScheduledDetect& detect) {
  if (!failure_detector_threadpool_->Offer(detect)) {
    stringstream ss;
    ss << "Failed to schedule detect task to thread pool.";
    LOG(ERROR) << ss.str();
    return Status::UnknownError(ss.str());
  }

  return Status::OK();
}

void WorkerFailureDetector::DoDetect(int thread_id,
    const ScheduledDetect& detect) {
  int64_t detect_deadline = detect.deadline;
  if (detect_deadline != 0) {
    // Wait until deadline.
    int64_t diff_ms = detect_deadline - UnixMillis();
    while (diff_ms > 0) {
      SleepForMs(diff_ms);
      diff_ms = detect_deadline - UnixMillis();
    }
    
    diff_ms = std::abs(diff_ms);
    LOG(INFO) << "Do failure detect: "
      << " (deadline accuracy: " << diff_ms << "ms)";

    if (diff_ms > DEADLINE_MISS_THRESHOLD_MS) {
      const string& msg = strings::Substitute(
          "Missed detect deadline by $0ms, "
          "consider increasing --worker_heartbeat_frequency_ms (currently $1)",
          diff_ms,
          FLAGS_worker_heartbeat_frequency_ms);
      LOG(WARNING) << msg;
    }
  } else {
    // Immediately detect has a deadline of 0. There's no need
    // to wait.
    LOG(INFO) << "Immediately detect.";
  }
  
  // Send the detect message, and compute the next deadline
  int64_t deadline_ms = 0;
  Status status = Detect();
  if (status.ok()) {
    //refresh the last detect timestamp
  }

  deadline_ms = UnixMillis() + FLAGS_worker_heartbeat_frequency_ms;
  
  // Schedule the next detect.
  LOG(INFO) << "Next detect deadline is in " << deadline_ms << "ms";
  status = OfferDetect(ScheduledDetect(deadline_ms));
  if (!status.ok()) {
    LOG(WARNING) << "Unable to schedule next detect: "
                  << status.message();
  }
}

Status WorkerFailureDetector::Detect() {
  // check all the workers and the last heartbeat time
  // if we don't see one last heartbeat during last worker_heartbeat_frequency_ms
  // we feedback as not seen
  std::vector<std::shared_ptr<WorkerRegistration>> registrations;
  worker_manager_->GetWorkerRegistrations(registrations);
  typedef std::vector<std::shared_ptr<WorkerRegistration>> WorkerRegistrations;
  for(WorkerRegistrations::iterator it = registrations.begin();
    it != registrations.end(); ++it) {
    DetectWorker((*it)->id());
  }
  return Status::OK();
}

Status WorkerFailureDetector::DetectWorker(const std::string& id)  {
  bool seen = false;

  {
    lock_guard<mutex> l(worker_last_heartbeat_lock_);
    map<string, system_time>::iterator heartbeat_record = worker_last_heartbeat_.find(id);
    if (heartbeat_record != worker_last_heartbeat_.end()) {
      // see a record check the time
      time_duration duration = get_system_time() - heartbeat_record->second;
      if (duration <= milliseconds(FLAGS_worker_heartbeat_frequency_ms)) {
        seen = true;
      }
    }
  }
  
  FailureDetector::PeerState state = 
    failure_detector_->UpdateHeartbeat(id, seen);
    
  if (state == FailureDetector::FAILED) {
    // notify the worker manager that the worker is dead
    worker_manager_->OnWorkerFailed(id);
  }
  return Status::OK();
}

Status WorkerFailureDetector::UpdateHeartbeat(const std::string& peer) {
  // record the last heartbeat time of the worker
  {
    lock_guard<mutex> l(worker_last_heartbeat_lock_);
    worker_last_heartbeat_[peer] = get_system_time();
  }
  return Status::OK();
}

FailureDetector::PeerState WorkerFailureDetector::GetPeerState(const std::string& peer) {
  return failure_detector_->GetPeerState(peer);
}

/// Remove a peer from the failure detector completely.
void WorkerFailureDetector::EvictPeer(const std::string& peer) {
  {
    lock_guard<mutex> l(worker_last_heartbeat_lock_);
    worker_last_heartbeat_.erase(peer);
  }
  failure_detector_->EvictPeer(peer);
}

} // namespace pegasus