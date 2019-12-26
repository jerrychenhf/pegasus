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

ConsistentHashRing::ConsistentHashRing(std::vector<Location> locations)
{
	if (NULL == conhash)
	{
		conhash = conhash_init(NULL);
	}

	for (auto lcn:locations) {
		AddLocation(lcn);
	}
}

void ConsistentHashRing::AddLocation(Location location)
{
	//TODO: set a proper value for num_vn
	int num_vn = 10;
	AddLocation(location, num_vn);
}

void ConsistentHashRing::AddLocation(Location location, int num_virtual_nodes)
{
	struct node_s node;
	conhash_set_node(&node, location.ToString().c_str(), num_virtual_nodes);
	conhash_add_node(conhash, &node);
}

void ConsistentHashRing::RemoveLocation(Location location)
{
	//TODO: the libcohash needs update to remove dependency on node
	//TODO: how to get the num_virtual_nodes?
	struct node_s node;
	conhash_set_node(&node, location.ToString().c_str(), 10);
	conhash_del_node(conhash, &node);
}

Location ConsistentHashRing::GetLocation(Identity identity)
{
	const struct node_s* pnode;
	std::string idstr;
	identity.SerializeToString(&idstr);
	pnode = conhash_lookup(conhash, idstr.c_str());
	//TODO: refactor lcn.Parse?
	Location lcn;
	lcn.Parse(idstr, &lcn);
	return lcn;
}

} // namespace pegasus
