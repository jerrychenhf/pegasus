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

#ifndef PEGASUS_WORKER_MANAGER_H
#define PEGASUS_WORKER_MANAGER_H

#include <atomic>
#include <vector>

#include <boost/thread/mutex.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/unordered_map.hpp>

#include "common/status.h"
#include "common/location.h"
#include "rpc/types.h"
#include "server/planner/worker_failure_detector.h"

using namespace std;

namespace pegasus {

class WorkerFailureDetector;

/// A SubscriberId uniquely identifies a single subscriber, and is
/// provided by the subscriber at registration time.
typedef std::string WorkerId;
    
class WorkerRegistration {
public:
  enum WorkerState {
    UNKNOWN = 0,  /// Unused
    ACTIVE = 1,
    DEAD = 2
  };
  
  WorkerRegistration(const WorkerId& id)
    : id_(id), state_(WorkerState::UNKNOWN), last_heartbeat_time_(0) {
  }
  
  const WorkerId& id() const { return id_; }
  const rpc::Location& address() const { return address_; }
  WorkerState state() const { return state_; }
  std::shared_ptr<rpc::NodeInfo> node_info() const { return node_info_; }

public:
  WorkerId id_;
  rpc::Location address_;
  
  WorkerState state_;
  int64_t last_heartbeat_time_;
  
  // concurrent update and access
  // the internal pointer may be updated
  std::shared_ptr<rpc::NodeInfo> node_info_;
};

// Get the worker locations
class WorkerManager {
 public:
  WorkerManager();
  
  Status Init();
  
  Status GetWorkerRegistrations(std::vector<std::shared_ptr<WorkerRegistration>>& registrations);

  Status Heartbeat(const rpc::HeartbeatInfo& info, std::unique_ptr<rpc::HeartbeatResult>* result);
  
  // Notified by failure detector that the worker failed
  Status OnWorkerFailed(const WorkerId& id);
  
 private:
  std::shared_ptr<std::vector<std::shared_ptr<Location>>> locations;
  
  Status RegisterWorker(const rpc::HeartbeatInfo& info);
  Status HeartbeatWorker(const rpc::HeartbeatInfo& info);
  
  Status UnregisterWorker(const WorkerId& id);
  
  typedef boost::unordered_map<WorkerId, std::shared_ptr<WorkerRegistration>>
    WorkerRegistrationMap;
  WorkerRegistrationMap workers_;
  
  /// Protects access to workers
  boost::mutex workers_lock_;
  
  boost::scoped_ptr<WorkerFailureDetector> worker_failure_detector_;
};

} // namespace pegasus

#endif  // PEGASUS_WORKER_MANAGER_H

