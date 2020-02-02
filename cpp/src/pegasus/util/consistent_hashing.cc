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

#include "pegasus/util/consistent_hashing.h"

namespace pegasus {

struct conhash_s* ConsistentHashRing::conhash = NULL;

ConsistentHashRing::ConsistentHashRing()
{
//	distpolicy_ = CONHASH;
}

ConsistentHashRing::~ConsistentHashRing()
{
	if (NULL != ConsistentHashRing::conhash)
	{
		conhash_fini(ConsistentHashRing::conhash);
	}
}

//
void ConsistentHashRing::PrepareValidLocations(std::shared_ptr<std::vector<std::shared_ptr<Location>>> locations)
{
	// If the locations are not provided, get the worker locations from worker_manager
	if (nullptr != locations)
	{
		validlocations_ = locations;
	}
	else
	{
		//TODO: get worker_manager first
		std::shared_ptr<std::vector<std::shared_ptr<Location>>> worker_locations;
//		worker_manager_->GetWorkerLocations(worker_locations);
		validlocations_ = worker_locations;
	}
}

void ConsistentHashRing::SetupDist()
{
	if (NULL == ConsistentHashRing::conhash)
	{
		ConsistentHashRing::conhash = conhash_init(NULL);
	}
//	std::vector<std::shared_ptr<Location>> lcns;
	for (auto lcn:(*validlocations_)) {
		AddLocation(*lcn);
	}
}

void ConsistentHashRing::AddLocation(Location location)
{
	int num_vn = location.GetCacheSize()/10;
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
	std::string idstr = identity.file_path();
//	identity.SerializeToString(&idstr);
	pnode = conhash_lookup(conhash, idstr.c_str());
	// create the location object and fill with phynode's location (uri).
	Location lcn;
	lcn.Parse(pnode->iden, &lcn);  	//TODO: refactor Location::Parse()?
	return lcn;
}

void ConsistentHashRing::GetDistLocations(std::shared_ptr<std::vector<Identity>> vectident, std::shared_ptr<std::vector<Location>> vectloc)
{
//	std::vector<Location> vectloc;
	const struct node_s* pnode;
	for (auto ident:(*vectident))
	{
		std::string idstr = ident.file_path();
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
//	for (auto ident:(*vectident))
	{
		std::string idstr = partt.GetIdentPath();
		pnode = conhash_lookup(conhash, idstr.c_str());
		// create the location object and fill with phynode's location (uri).
		Location lcn;
		lcn.Parse(pnode->iden, &lcn);  	//TODO: refactor Location::Parse()?
		partt.UpdateLocation(lcn);
	}
}

} // namespace pegasus
