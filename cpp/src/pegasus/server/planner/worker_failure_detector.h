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

#ifndef PEGASUS_WORKER_FAILURE_DETECTOR_H
#define PEGASUS_WORKER_FAILURE_DETECTOR_H

#include <atomic>
#include <boost/thread/mutex.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include "common/status.h"
#include "server/planner/failure-detector.h"

using namespace std;

namespace pegasus {

class WorkerManager;

template <typename T>
class ThreadPool;

class WorkerFailureDetector {
 public:
  WorkerFailureDetector(WorkerManager* worker_manager);
  ~WorkerFailureDetector();
  
  Status Init();

  Status Start();
  Status Stop();
  
  Status UpdateHeartbeat(const std::string& peer);

  /// Returns the current estimated state of a peer.
  FailureDetector::PeerState GetPeerState(const std::string& peer);

  /// Remove a peer from the failure detector completely.
  void EvictPeer(const std::string& peer);
  
 private:
  WorkerManager* worker_manager_;
 
  /// Work item passed to failure detector thread pool
  struct ScheduledDetect {
    /// *Earliest* time (in Unix time) that the next check should be done
    int64_t deadline;

    ScheduledDetect() {}
    
    ScheduledDetect(int64_t next_update_time): deadline(next_update_time) {}
  };
  
  std::shared_ptr<ThreadPool<ScheduledDetect>> failure_detector_threadpool_;
  
  /// Utility method to add an work to the given thread pool, and to fail if the thread
  /// pool is already at capacity.
  Status OfferDetect(const ScheduledDetect& heartbeat) WARN_UNUSED_RESULT;

  /// Sends a heartbeat update, Once complete, the next update is scheduled and
  /// added to the appropriate queue.
  void DoDetect(int thread_id,
      const ScheduledDetect& heartbeat);

  /// Failure detector for workers. If a worker misses a configurable number of
  /// consecutive heartbeat messages, it is considered failed and its entry in 
  /// the worker map is erased. The worker hostname is used to identify peers 
  /// for failure detection purposes. Worker state is evicted from the failure
  /// detector when the worker is unregistered, so old workers do not occupy 
  /// memory and the failure detection state does not
  /// carry over to any new registrations of the previous worker.
  boost::scoped_ptr<MissedHeartbeatFailureDetector> failure_detector_;
  
  Status Detect();
  Status DetectWorker(const std::string& id);
  
  /// Protects all members
  boost::mutex worker_last_heartbeat_lock_;

  /// Record of last time a successful heartbeat was received
  std::map<std::string, boost::system_time> worker_last_heartbeat_;
};

} // namespace pegasus

#endif  // PEGASUS_WORKER_FAILURE_DETECTOR_H
