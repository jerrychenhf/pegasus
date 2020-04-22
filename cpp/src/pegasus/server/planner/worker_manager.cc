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

#include "server/planner/worker_manager.h"
#include "util/time.h"
#include <boost/thread/lock_guard.hpp>
#include "gutil/strings/substitute.h"
#include "server/planner/worker_failure_detector.h"

using namespace boost;

namespace pegasus {
WorkerManager::WorkerManager() {
  worker_failure_detector_.reset(new WorkerFailureDetector(this));
}

Status WorkerManager::Init() {
  RETURN_IF_ERROR(worker_failure_detector_->Init());
  RETURN_IF_ERROR(worker_failure_detector_->Start());
  return Status::OK();
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
                                std::unique_ptr<rpc::HeartbeatResult>* result) {
  Status status = Status::OK();
  if(info.type == rpc::HeartbeatInfo::REGISTRATION) {
    status = RegisterWorker(info);
  } else if(info.type == rpc::HeartbeatInfo::HEARTBEAT) {
    status = HeartbeatWorker(info);
  }
  RETURN_IF_ERROR(status);

  auto hbrc = std::make_shared<rpc::HeartbeatResultCmd>();
  std::unique_ptr<rpc::HeartbeatResult> r =
    std::unique_ptr<rpc::HeartbeatResult>(new rpc::HeartbeatResult());
  if(info.type == rpc::HeartbeatInfo::REGISTRATION) {
    r->result_code = rpc::HeartbeatResult::REGISTERED;
  } else  if(info.type == rpc::HeartbeatInfo::HEARTBEAT) {
    r->result_code = rpc::HeartbeatResult::HEARTBEATED;
    if (NeedtoDropCache(info.hostname))
    {
      LOG(INFO) << "Yes, " << info.hostname << " has data cache to drop. Building rpc cmd...";
      hbrc->hbrc_action = rpc::HeartbeatResultCmd::DROPCACHE;
      auto sppartlist = std::make_shared<WorkerCacheDropList>();
      RETURN_IF_ERROR(GetCacheDropList(info.hostname, sppartlist));
      hbrc->hbrc_parameters = sppartlist->GetPartstodrop();
      r->result_hascmd = true;
      r->result_command = std::move(*hbrc);
    } else {
      LOG(INFO) << "No need to drop cache for " << info.hostname;
      r->result_hascmd = false;
    }
  } else {
    r->result_code = rpc::HeartbeatResult::UNKNOWN;
  }
  
  *result = std::move(r);
  return Status::OK();
}


Status WorkerManager::RegisterWorker(const rpc::HeartbeatInfo& info) {
  WorkerId id = info.hostname;
  std::string address_str;
  
  LOG(INFO) << "Registering worker: " << id;
  {
    lock_guard<mutex> l(workers_lock_);
    WorkerRegistrationMap::iterator worker_it = workers_.find(id);
    if (worker_it != workers_.end()) {
      UnregisterWorker(id);
    }
    
    std::shared_ptr<WorkerRegistration> current_registration(
        new WorkerRegistration(id));
    std::shared_ptr<Location> address = info.get_address();
    if(address == nullptr) {
      return Status::Invalid("Address is not specified for registration");
    }

    address_str = address->ToString();
    current_registration->address_ = *(address.get());
    current_registration->state_ = WorkerRegistration::ACTIVE;
    current_registration->last_heartbeat_time_ = UnixMillis();
    
    if (info.node_info != nullptr) {
      // there are node info update
      LOG(INFO) << "Worker '" << id <<  " updated node info.";
      current_registration->node_info_ = info.node_info;
    }
    
    workers_.emplace(id, current_registration);
    
    worker_failure_detector_->UpdateHeartbeat(id);
  }

  NotifyObservers(WMEVENT_WORKERNODE_ADDED);
  LOG(INFO) << "Worker '" << id << "' registered with address: " << address_str;
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
      return Status::ObjectNotFound(strings::Substitute("Worker $0 not found.", id));
    }
    
    WorkerRegistration* current_registration = worker_it->second.get();
    current_registration->state_ = WorkerRegistration::ACTIVE;
    current_registration->last_heartbeat_time_ = UnixMillis();
    
    if (info.node_info != nullptr) {
      // there are node info update
      LOG(INFO) << "Worker '" << id <<  " updated node info.";
      current_registration->node_info_ = info.node_info;
      LOG(INFO) << "dataset cache hit rate: " \
                << info.node_info->ds_cacherd_cnt*100/info.node_info->total_cacherd_cnt\
                << "% (" << info.node_info->ds_cacherd_cnt << "/" << info.node_info->total_cacherd_cnt << ")";
      LOG(INFO) << "partition cache hit rate: " \
                << info.node_info->pt_cacherd_cnt*100/info.node_info->total_cacherd_cnt\
                << "% (" << info.node_info->pt_cacherd_cnt << "/" << info.node_info->total_cacherd_cnt << ")";
      LOG(INFO) << "column cache hit rate: " \
                << info.node_info->col_cacherd_cnt*100/info.node_info->total_cacherd_cnt\
                << "% (" << info.node_info->col_cacherd_cnt << "/" << info.node_info->total_cacherd_cnt << ")";
/*      LOG(INFO) << "\ttotal_cacherd_cnt: " << current_registration->node_info_->total_cacherd_cnt;
      LOG(INFO) << "\tds_cacherd_cnt: " << current_registration->node_info_->ds_cacherd_cnt;
      LOG(INFO) << "\tpt_cacherd_cnt: " << current_registration->node_info_->pt_cacherd_cnt;
      LOG(INFO) << "\tcol_cacherd_cnt: " << current_registration->node_info_->col_cacherd_cnt;
*/    }
    
    worker_failure_detector_->UpdateHeartbeat(id);
    
  }
  
  return Status::OK();
}

Status WorkerManager::UnregisterWorker(const WorkerId& id) {
  // already in lock
  WorkerRegistrationMap::const_iterator it = workers_.find(id);
  if (it == workers_.end()) {
    // Already failed and / or replaced with a new registration
    return Status::OK();
  }

  // Prevent the failure detector from growing without bound
  worker_failure_detector_->EvictPeer(id);

  workers_.erase(id);

  NotifyObservers(WMEVENT_WORKERNODE_REMOVED);
  return Status::OK();
}

Status WorkerManager::OnWorkerFailed(const WorkerId& id) {
  LOG(INFO) << "Worker failure: " << id;
  {
    lock_guard<mutex> l(workers_lock_);
    UnregisterWorker(id);
  }
  
  return Status::OK();
}

Status WorkerManager::UpdateCacheDropLists(std::shared_ptr<std::vector<Partition>> partits) {

  // concurrent control
  lock_guard<mutex> l(mapcachedrop_lock_);

  // for each partition, add it to corresponding worker-cachedroplist
  for (auto part : (*partits))
  {
    // get workerid (location hostname)
    WorkerId wkid = part.GetLocationHostname();
    // add this partition to WorkerCacheDropList, first check if it exists
    auto it = mapwkcachedroplist_.find(wkid);
    if (it == mapwkcachedroplist_.end())
    {
      mapwkcachedroplist_[wkid] = std::make_shared<WorkerCacheDropList>();
    }
    mapwkcachedroplist_[wkid]->InsertPartition(part);
  }

  // debug output
  LOG(INFO) << "mapwkcachedroplist_:";
  for (auto it : mapwkcachedroplist_) {
    LOG(INFO) << "\t" << it.first << ", with " << it.second->GetPartstodrop().size() << " partition(s).";
  }

  return Status::OK();
}

bool WorkerManager::NeedtoDropCache(const WorkerId& id) {
  LOG(INFO) << "Checking if " << id << " has partitions to drop...";

  lock_guard<mutex> l(mapcachedrop_lock_);

  auto it = mapwkcachedroplist_.find(id);
  if (it != mapwkcachedroplist_.end())
    return true;
  else
    return false;
}

Status WorkerManager::GetCacheDropList(const WorkerId& id, std::shared_ptr<WorkerCacheDropList>& sppartlist) {

  // concurrent control
  lock_guard<mutex> l(mapcachedrop_lock_);

  auto it = mapwkcachedroplist_.find(id);
  if (it != mapwkcachedroplist_.end())
  {
    LOG(INFO) << "Fetching " << id << " from mapwkcachedroplist_...";
    sppartlist = std::move(mapwkcachedroplist_[id]);
    mapwkcachedroplist_.erase(it);
    LOG(INFO) << "Fetched.";
  }

  return Status::OK();
}

} // namespace pegasus