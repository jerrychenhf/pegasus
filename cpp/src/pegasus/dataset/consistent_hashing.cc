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
#include <iomanip>
#include "consistent_hashing.h"
#include "runtime/planner_exec_env.h"
#include "server/planner/worker_manager.h"

DECLARE_int32(max_virtual_node_num);

namespace pegasus
{

ConsistentHashRing::ConsistentHashRing()
{
	valid_locations_ = nullptr;
	node_cache_capacity_ = nullptr;
}

ConsistentHashRing::~ConsistentHashRing()
{
}

//TODO: GetValidLocations(), then assign them.
void ConsistentHashRing::PrepareValidLocations(std::shared_ptr<std::vector<Location>> locations,
											   std::shared_ptr<std::vector<int64_t>> node_cache_capacity)
{
	//TODO: return early, reduce nested code
	if (nullptr != locations)
	{
		valid_locations_ = locations;
		node_cache_capacity_ = node_cache_capacity;
	}
	else // If the locations are not provided, get the worker locations from worker_manager
	{
		std::shared_ptr<WorkerManager> worker_manager = PlannerExecEnv::GetInstance()->get_worker_manager();
		std::vector<std::shared_ptr<WorkerRegistration>> wregs;
		worker_manager->GetWorkerRegistrations(wregs);
		LOG(INFO) << "node count from workerregistration vector: " << wregs.size();
		if (wregs.size() > 0)
		{
			valid_locations_ = std::make_shared<std::vector<Location>>();
			node_cache_capacity_ = std::make_shared<std::vector<int64_t>>();
			for (auto it : wregs)
			{
				valid_locations_->push_back(it->address());
				LOG(INFO) << "- insert location: " << it->address().ToString();
				node_cache_capacity_->push_back(it->node_info()->get_cache_capacity() / (1024 * 1024));
				LOG(INFO) << "- nodecachesize(MB): " << it->node_info()->get_cache_capacity() / (1024 * 1024);
			}
		}
	}
}

Status ConsistentHashRing::SetupDistribution()
{
	LOG(INFO) << "SetupDistribution()...";
	if (valid_locations_)
	{
		for (unsigned int i = 0; i < valid_locations_->size(); i++)
		{
			AddLocation(i);
			LOG(INFO) << "Added location: #" << i;
		}
		LOG(INFO) << "Iterate the conhash:";
		ConHashMetrics chmetrics;
		for (auto it = consistent_hash_.begin(); it != consistent_hash_.end(); ++it)
		{
			LOG(INFO) << "node: " << it->second << "\t" << std::right << std::setw(10) << it->first;
			std::size_t pos = it->second.find_last_of("_");
			chmetrics.Increment(it->second.substr(0, pos));
		}
		chmetrics.WriteAsJson();
		return Status::OK();
	}
	else
	{
		LOG(ERROR) << "Error! ConsistentHashRing has 0 locations. Call PrepareValidLocations() first.";
		return Status::Invalid("Error! None valid location in ConsistentHashRing.");
	}
}

void ConsistentHashRing::AddLocation(unsigned int locationid)
{
	int vnodecount = node_cache_capacity_->at(locationid) / VIRT_NODE_DIVISOR;
	vnodecount = std::max(MIN_VIRT_NODE_NUM, vnodecount);
	vnodecount = std::min(FLAGS_max_virtual_node_num, vnodecount);

	for (int i = 0; i < vnodecount; i++)
	{
		std::string node = valid_locations_->at(locationid).ToString() + "_" + std::to_string(i);
		//LOG(INFO) << "consistent_hash_.insert(" << node << ");";
		consistent_hash_.insert(node);
	}
}
#if 0
void ConsistentHashRing::AddLocation(Location location)
{

	int vnodecount = node_cache_capacity_ / 1000;
	vnodecount = std::max(MIN_VIRT_NODE_NUM, vnodecount);
	vnodecount = std::min(MAX_VIRT_NODE_NUM, vnodecount);
	AddLocation(location, vnodecount);
}
void ConsistentHashRing::AddLocation(Location location, int num_virtual_nodes)
{
#if 0
	std::string node = location.ToString() + "_" + std::to_string(num_virtual_nodes);
	consistent_hash_.insert(node);
#endif
}
#endif
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

void ConsistentHashRing::GetDistLocations(std::shared_ptr<std::vector<Identity>> identities,
										  std::shared_ptr<std::vector<Location>> locations)
{
}

void ConsistentHashRing::GetDistLocations(std::shared_ptr<std::vector<Partition>> partitions)
{
	crc32_hasher h;
	ConHashMetrics chm;
	for (auto partition : (*partitions))
	{
		LOG(INFO) << "h(partition.GetIdentityPath()): " << h(partition.GetIdentityPath());
		consistent_hash_t::iterator it;
		it = consistent_hash_.find(h(partition.GetIdentityPath()));
		LOG(INFO) << "found: " << it->second;
		std::size_t pos = it->second.find_last_of("_");
		std::string node = it->second.substr(0, pos);
		//		LOG(INFO) << node;
		// create the location object and fill with phynode's location (uri).
		//		Location lcn;
		//		Location::Parse(node, &lcn);
		//std::cout << "lcn.ToString(): " << lcn.ToString() << std::endl;
		//		partition.UpdateLocation(lcn);
		partition.UpdateLocationURI(node);
		chm.Increment(node);
	}
	chm.WriteToLog();
}

#if 0
void ConsistentHashRing::GetDistLocations(std::shared_ptr<std::vector<Identity>> identities, \
								std::shared_ptr<std::vector<Location>> locations)
{
	const struct node_s* pnode;
	for (auto ident:(*identities))
	{
		std::string idstr = ident.partition_id();
		pnode = conhash_lookup(conhash, idstr.c_str());
		// create the location object and fill with phynode's location (uri).
		Location lcn;
		lcn.Parse(pnode->iden, &lcn);  	//TODO: refactor Location::Parse()?
		locations->push_back(lcn);
	}
}
#endif
} // namespace pegasus
