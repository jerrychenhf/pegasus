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
//    boost::format node_fmt("192.168.1.%1%");

	validlocations_ = nullptr;
	nodecacheMB_ = nullptr;
}

ConsistentHashRing::~ConsistentHashRing()
{
}

void ConsistentHashRing::PrepareValidLocations(std::shared_ptr<std::vector<Location>> locations, 
											std::shared_ptr<std::vector<int>> nodecacheMB)
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
		std::cout << "node count form workerregistration vector: " << wregs.size() << std::endl;
		for (auto it:wregs)
		{
			validlocations_->push_back(it->address());
//			nodecacheMB_->push_back(it->getcachesizeMB());	// TODO: need implementation in WorkerRegistration.
			//fake code for test
			nodecacheMB_->push_back(1024);	//1GB
		}
	}
}

void ConsistentHashRing::SetupDist()
{
//	std::vector<std::shared_ptr<Location>> lcns;
	if (validlocations_)
	{
//		for (auto lcn:(*validlocations_)) {
//			AddLocation(lcn);
//		}
		for (unsigned int i=0; i<validlocations_->size(); i++)
			AddLocation(i);
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
	std::cout << "h(identity.partition_id()): " << h(identity.partition_id()) << endl;
    consistent_hash_t::iterator it;
    it = consistent_hash_.find(h(identity.partition_id()));
    std::cout<<boost::format("node:%1%, %2%") % it->second % it->first << std::endl;

	std::size_t pos = it->second.find_last_of("_");
	std::string node = it->second.substr(0, pos);
	std::cout << node << std::endl;
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
		std::cout << "h(partt.GetIdentPath()): " << h(partt.GetIdentPath()) << endl;
	    consistent_hash_t::iterator it;
    	it = consistent_hash_.find(h(partt.GetIdentPath()));
		std::size_t pos = it->second.find_last_of("_");
		std::string node = it->second.substr(0, pos);
		std::cout << node << std::endl;
		// create the location object and fill with phynode's location (uri).
		Location lcn;
		Location::Parse(node, &lcn);
		partt.UpdateLocation(lcn);
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
