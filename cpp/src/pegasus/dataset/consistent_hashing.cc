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

#include "consistent_hashing.h"
#include "runtime/planner_exec_env.h"
#include "server/planner/worker_manager.h"

namespace pegasus {

struct conhash_s* ConsistentHashRing::conhash = NULL;

ConsistentHashRing::ConsistentHashRing()
{
//	distpolicy_ = CONHASH;
	validlocations_ = nullptr;
}

ConsistentHashRing::~ConsistentHashRing()
{
	if (NULL != ConsistentHashRing::conhash)
	{
		conhash_fini(ConsistentHashRing::conhash);
	}
}

// TODO: decouple it with workermanager
void ConsistentHashRing::PrepareValidLocations(std::shared_ptr<std::vector<std::shared_ptr<Location>>> locations)
{
	// If the locations are not provided, get the worker locations from worker_manager
	if (nullptr != locations)
	{
		validlocations_ = locations;
	}
	else
	{
		std::shared_ptr<WorkerManager> worker_manager = PlannerExecEnv::GetInstance()->GetInstance()->get_worker_manager();
//		std::shared_ptr<std::vector<std::shared_ptr<Location>>> worker_locations;
		//Status WorkerManager::GetWorkerRegistrations(std::vector<std::shared_ptr<WorkerRegistration>>& registrations)
		std::vector<std::shared_ptr<WorkerRegistration>> wregs;
		worker_manager->GetWorkerRegistrations(wregs);
		for (auto &it:wregs)
		{
			validlocations_->push_back(std::make_shared<Location>(it->address()));
		}
	}
}

void ConsistentHashRing::SetupDist()
{
	if (NULL == ConsistentHashRing::conhash)
	{
		ConsistentHashRing::conhash = conhash_init(NULL);
	}
//	std::vector<std::shared_ptr<Location>> lcns;
	if (validlocations_)
	{
		for (auto lcn:(*validlocations_)) {
			AddLocation(*lcn);
		}
	}
}

void ConsistentHashRing::AddLocation(Location location)
{
	//TO CORRECT: get from worker registation instead of Location
	uint32_t cacheSize = 512; //GB
	int num_vn = cacheSize / 10;
	num_vn = std::max(MIN_VIRT_NODE_NUM, num_vn);
	num_vn = std::min(MAX_VIRT_NODE_NUM, num_vn);
	AddLocation(location, num_vn);
}

void ConsistentHashRing::AddLocation(Location location, int num_virtual_nodes)
{
	struct node_s* pnode = new (struct node_s);
	conhash_set_node(pnode, location.ToString().c_str(), num_virtual_nodes);
	conhash_add_node(conhash, pnode);
}
#if 0
void ConsistentHashRing::RemoveLocation(Location location)
{
	//TODO: the libcohash needs update to remove dependency on node
	struct node_s node;
	conhash_set_node(&node, location.ToString().c_str(), MAX_VIRT_NODE_NUM);
	conhash_del_node(conhash, &node);
}
#endif
Location ConsistentHashRing::GetLocation(Identity identity)
{
	const struct node_s* pnode;
	std::string idstr = identity.partition_id();
//	identity.SerializeToString(&idstr);
	pnode = conhash_lookup(conhash, idstr.c_str());
	// create the location object and fill with phynode's location (uri).
	Location lcn;
	lcn.Parse(pnode->iden, &lcn);  	//TODO: refactor Location::Parse()?
	return lcn;
}

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

void ConsistentHashRing::GetDistLocations(std::shared_ptr<std::vector<Partition>> partitions)
{
	const struct node_s* pnode;
	for (auto partt:(*partitions))
	{
		std::string idstr = partt.GetIdentPath();
		pnode = conhash_lookup(conhash, idstr.c_str());
		// create the location object and fill with phynode's location (uri).
		Location lcn;
		lcn.Parse(pnode->iden, &lcn);
		partt.UpdateLocation(lcn);
	}
}

} // namespace pegasus
