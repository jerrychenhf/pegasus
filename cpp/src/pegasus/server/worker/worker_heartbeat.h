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

#ifndef PEGASUS_WORKER_HEARTBEAT_H
#define PEGASUS_WORKER_HEARTBEAT_H

#include "common/status.h"

using namespace std;

namespace pegasus {

template <typename T>
class ThreadPool;

class WorkerHeartbeat {
 public:
  WorkerHeartbeat();
  ~WorkerHeartbeat();
  
  Status Init();
  Status Start();
  Status Stop();

 private:
 
  /// Work item passed to heartbeat thread pool
  struct ScheduledHeartbeat {
    /// *Earliest* time (in Unix time) that the next message should be sent.
    int64_t deadline;

    ScheduledHeartbeat() {}

    ScheduledHeartbeat(int64_t next_update_time): deadline(next_update_time) {}
  };
  
  std::shared_ptr<ThreadPool<ScheduledHeartbeat>> heartbeat_threadpool_;
  
  /// Utility method to add an work to the given thread pool, and to fail if the thread
  /// pool is already at capacity.
  Status OfferHeartbeat(const ScheduledHeartbeat& heartbeat) WARN_UNUSED_RESULT;

  /// Sends a heartbeat update, Once complete, the next update is scheduled and
  /// added to the appropriate queue.
  void DoHeartbeat(int thread_id,
      const ScheduledHeartbeat& heartbeat);

  /// Sends a heartbeat message to planner. Returns false if there was some error
  /// performing the RPC.
  Status SendHeartbeat() WARN_UNUSED_RESULT;

};

} // namespace pegasus

#endif  // PEGASUS_WORKER_HEARTBEAT_H