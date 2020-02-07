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

#include "common/worker_manager.h"
#include "util/time.h"
#include <boost/thread/lock_guard.hpp>
#include "gutil/strings/substitute.h"

using namespace boost;

namespace pegasus {
WorkerManager::WorkerManager() {

}

Status WorkerManager::GetWorkerRegistrations(
  std::vector<std::shared_ptr<WorkerRegistration>>& registrations) {
  {
    lock_guard<mutex> l(workers_lock_);
    for(WorkerRegistrationMap::iterator it = workers_.begin();
      it != workers_.end(); ++it) {
      registrations.push_back(it->second);
    }
  }
  return Status::OK();
}

Status WorkerManager::Heartbeat(const rpc::HeartbeatInfo& info,
  std::unique_ptr<rpc::HeartbeatResult>* result){
  Status status = Status::OK();
  if(info.type == rpc::HeartbeatInfo::REGISTRATION) {
    status = RegisterWorker(info);
  } else if(info.type == rpc::HeartbeatInfo::HEARTBEAT) {
    status = HeartbeatWorker(info);
  }
  RETURN_IF_ERROR(status);
  
  std::unique_ptr<rpc::HeartbeatResult> r =
    std::unique_ptr<rpc::HeartbeatResult>(new rpc::HeartbeatResult());
  if(info.type == rpc::HeartbeatInfo::REGISTRATION) {
    r->result_code = rpc::HeartbeatResult::REGISTERED;
  } else  if(info.type == rpc::HeartbeatInfo::HEARTBEAT) {
    r->result_code = rpc::HeartbeatResult::HEARTBEATED;
  } else {
    r->result_code = rpc::HeartbeatResult::UNKNOWN;
  }
  
  *result = std::move(r);
  return Status::OK();
}

Status WorkerManager::RegisterWorker(const rpc::HeartbeatInfo& info) {
  WorkerId id = info.hostname;
  
  LOG(INFO) << "Registering worker: " << id;
  {
    lock_guard<mutex> l(workers_lock_);
    WorkerRegistrationMap::iterator worker_it = workers_.find(id);
    if (worker_it != workers_.end()) {
      UnregisterWorker(worker_it->second.get());
    }
    
    std::shared_ptr<WorkerRegistration> current_registration(
        new WorkerRegistration(id));
    current_registration->address_ = info.get_address();
    current_registration->state_ = WorkerRegistration::ACTIVE;
    current_registration->last_heartbeat_time_ = UnixMillis();
    workers_.emplace(id, current_registration);
    
    // TO DO : handle failing workers
    //failure_detector_->UpdateHeartbeat(subscriber_id, true);
  }

  LOG(INFO) << "Worker '" << id << "' registered.";
  return Status::OK();
}

Status WorkerManager::HeartbeatWorker(const rpc::HeartbeatInfo& info) {
  WorkerId id = info.hostname;
  
  VLOG(3) << "Heartbeat worker: " << id;
  {
    lock_guard<mutex> l(workers_lock_);
    WorkerRegistrationMap::iterator worker_it = workers_.find(id);
    if (worker_it == workers_.end()) {
      // worker not found
      return Status::ObjectNotFound(strings::Substitute("Worker $0 not found."));
    }
    
    WorkerRegistration* current_registration = worker_it->second.get();
    current_registration->state_ = WorkerRegistration::ACTIVE;
    current_registration->last_heartbeat_time_ = UnixMillis();
  }
  
  return Status::OK();
}

Status WorkerManager::UnregisterWorker(WorkerRegistration* worker) {
  // already in lock
  WorkerRegistrationMap::const_iterator it = workers_.find(worker->id());
  if (it == workers_.end()) {
    // Already failed and / or replaced with a new registration
    return Status::OK();
  }

  // TO DO : failure dectector
  // Prevent the failure detector from growing without bound
  //failure_detector_->EvictPeer(worker->id());

  workers_.erase(worker->id());
  return Status::OK();
}

} // namespace pegasus