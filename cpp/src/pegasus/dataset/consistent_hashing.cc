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

#include <iostream>
#include "consistent_hashing.h"
#include "runtime/planner_exec_env.h"
#include "server/planner/worker_manager.h"

namespace pegasus {


ConsistentHashRing::ConsistentHashRing()
{
	validlocations_ = nullptr;
	nodecacheMB_ = nullptr;
}

ConsistentHashRing::~ConsistentHashRing()
{
}

void ConsistentHashRing::PrepareValidLocations(std::shared_ptr<std::vector<Location>> locations, 
											std::shared_ptr<std::vector<int64_t>> nodecacheMB)
{
	if (nullptr != locations)
	{
		validlocations_ = locations;
		nodecacheMB_ = nodecacheMB;
	}
	else	// If the locations are not provided, get the worker locations from worker_manager
	{
		std::shared_ptr<WorkerManager> worker_manager = PlannerExecEnv::GetInstance()->GetInstance()->get_worker_manager();
		//Status WorkerManager::GetWorkerRegistrations(std::vector<std::shared_ptr<WorkerRegistration>>& registrations)
		std::vector<std::shared_ptr<WorkerRegistration>> wregs;
		worker_manager->GetWorkerRegistrations(wregs);
//std::cout << "node count from workerregistration vector: " << wregs.size() << std::endl;
LOG(INFO) << "node count from workerregistration vector: " << wregs.size();
		if (wregs.size() > 0)
		{
			validlocations_ = std::make_shared<std::vector<Location>>();
			nodecacheMB_ = std::make_shared<std::vector<int64_t>>();
			for (auto it:wregs)
			{
				validlocations_->push_back(it->address());
LOG(INFO) << "== insert location: " << it->address().ToString();
				nodecacheMB_->push_back(it->node_info()->get_cache_capacity()/(1024*1024));
LOG(INFO) << "== nodecachesize(MB): " << it->node_info()->get_cache_capacity()/(1024*1024);
//				nodecacheMB_->push_back(1024);
//LOG(INFO) << "== nodecachesize(MB): fake 1024";
			}
		}
	}
}

Status ConsistentHashRing::SetupDist()
{
	if (validlocations_)
	{
		for (unsigned int i=0; i<validlocations_->size(); i++)
			AddLocation(i);
		return Status::OK();
	}
	else
	{
		LOG(ERROR) << "Error! ConsistentHashRing has 0 locations. Call PrepareValidLocations() first.";
		return Status::Invalid("Error! None valid location in ConsistentHashRing.");
	}
	
}

void ConsistentHashRing::AddLocation(unsigned int locidx)
{
	int num_vn = nodecacheMB_->at(locidx) / 1000;
	num_vn = std::max(MIN_VIRT_NODE_NUM, num_vn);
	num_vn = std::min(MAX_VIRT_NODE_NUM, num_vn);

	std::string node = validlocations_->at(locidx).ToString() + "_" + std::to_string(num_vn);
	consistent_hash_.insert(node);
}

void ConsistentHashRing::AddLocation(Location location)
{
#if 0
	int num_vn = nodecacheMB_ / 1000;
	num_vn = std::max(MIN_VIRT_NODE_NUM, num_vn);
	num_vn = std::min(MAX_VIRT_NODE_NUM, num_vn);
	AddLocation(location, num_vn);
#endif
}
void ConsistentHashRing::AddLocation(Location location, int num_virtual_nodes)
{
#if 0
	std::string node = location.ToString() + "_" + std::to_string(num_virtual_nodes);
	consistent_hash_.insert(node);
#endif
}

Location ConsistentHashRing::GetLocation(Identity identity)
{
	crc32_hasher h;
	LOG(INFO) << "h(identity.partition_id()): " << h(identity.partition_id());
    consistent_hash_t::iterator it;
    it = consistent_hash_.find(h(identity.partition_id()));
    LOG(INFO) << boost::format("node:%1%, %2%") % it->second % it->first;

	std::size_t pos = it->second.find_last_of("_");
	std::string node = it->second.substr(0, pos);
	LOG(INFO) << node;
	// create the location object and fill with phynode's location (uri).
	Location lcn;
	Location::Parse(node, &lcn);
	return lcn;
}

void ConsistentHashRing::GetDistLocations(std::shared_ptr<std::vector<Identity>> vectident, \
								std::shared_ptr<std::vector<Location>> vectloc)
{
}

void ConsistentHashRing::GetDistLocations(std::shared_ptr<std::vector<Partition>> partitions)
{
	crc32_hasher h;
	for (auto partt:(*partitions))
	{
		LOG(INFO) << "h(partt.GetIdentPath()): " << h(partt.GetIdentPath());
	    consistent_hash_t::iterator it;
    	it = consistent_hash_.find(h(partt.GetIdentPath()));
		LOG(INFO) << "found: " << it->second;
		std::size_t pos = it->second.find_last_of("_");
		std::string node = it->second.substr(0, pos);
		LOG(INFO) << node;
		// create the location object and fill with phynode's location (uri).
//		Location lcn;
//		Location::Parse(node, &lcn);
//std::cout << "lcn.ToString(): " << lcn.ToString() << std::endl;
//		partt.UpdateLocation(lcn);
		partt.UpdateLocationURI(node);
	}
}

#if 0
void ConsistentHashRing::GetDistLocations(std::shared_ptr<std::vector<Identity>> vectident, \
								std::shared_ptr<std::vector<Location>> vectloc)
{
//	std::vector<Location> vectloc;
	const struct node_s* pnode;
	for (auto ident:(*vectident))
	{
		std::string idstr = ident.partition_id();
		pnode = conhash_lookup(conhash, idstr.c_str());
		// create the location object and fill with phynode's location (uri).
		Location lcn;
		lcn.Parse(pnode->iden, &lcn);  	//TODO: refactor Location::Parse()?
		vectloc->push_back(lcn);
	}
}
#endif
} // namespace pegasus
