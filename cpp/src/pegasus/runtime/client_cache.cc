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

#include "runtime/client_cache.h"

#include <sstream>
#include <memory>

#include <boost/thread/lock_guard.hpp>
#include "gutil/strings/substitute.h"
#include "common/logging.h"
#include "common/location.h"
#include "common/names.h"
#include "util/network-util.h"
#include "rpc/client.h"

namespace pegasus {
  
  pegasus::Status ClientWrapper::Open(uint32_t num_tries, uint64_t wait_ms) {
    std::string host;
    int32_t port = 0;
    
    // if (!pegasus::ParseAddress(address_, &host, &port))
    //   return Status::Invalid(strings::Substitute(
    //       "Address: $0 is invalid.",
    //       address_));
    
    rpc::Location location;
    // rpc::Status status = rpc::Location::ForGrpcTcp(host, port, &location);
    rpc::Status status = rpc::Location::ForGrpcUnix("/tmp/planner", &location);
    if(!status.ok())
      return pegasus::Status::fromArrowStatus(status);
    
    LOG(INFO) << "Trying to connect address: " << address_;
    status = rpc::FlightClient::Connect(location, &client_);
    if(!status.ok()) {
      return pegasus::Status::fromArrowStatus(status);
    }
    
    return pegasus::Status::OK();
  }
  
  pegasus::Status ClientCacheHelper::GetClient(const ClientAddress& address,
    ClientFactory factory_method, ClientKey* client_key) {
  shared_ptr<PerHostCache> host_cache;
  {
    lock_guard<mutex> lock(cache_lock_);
    VLOG(2) << "GetClient(" << ClientAddressToString(address) << ")";
    shared_ptr<PerHostCache>* ptr = &per_host_caches_[address];
    if (ptr->get() == NULL) ptr->reset(new PerHostCache());
    host_cache = *ptr;
  }

  {
    lock_guard<mutex> lock(host_cache->lock);
    if (!host_cache->clients.empty()) {
      *client_key = host_cache->clients.front();
      VLOG(2) << "GetClient(): returning cached client for " <<
          ClientAddressToString(address);
      host_cache->clients.pop_front();
      return pegasus::Status::OK();
    }
  }

  // Only get here if host_cache->clients.empty(). No need for the lock.
  RETURN_IF_ERROR(CreateClient(address, factory_method, client_key));
  return pegasus::Status::OK();
}

pegasus::Status ClientCacheHelper::ReopenClient(ClientFactory factory_method,
    ClientKey* client_key) {
  // Clients are not ordinarily removed from the cache completely (in the future, they may
  // be); this is the only method where a client may be deleted and replaced with another.
  shared_ptr<ClientWrapper> client_impl;
  ClientMap::iterator client;
  {
    lock_guard<mutex> lock(client_map_lock_);
    client = client_map_.find(*client_key);
    DCHECK(client != client_map_.end());
    client_impl = client->second;
  }
  VLOG(1) << "ReopenClient(): re-creating client for " <<
      ClientAddressToString(client_impl->address());

  client_impl->Close();

  // TODO: Thrift TBufferedTransport cannot be re-opened after Close() because it does not
  // clean up internal buffers it reopens. To work around this issue, create a new client
  // instead.
  ClientKey old_client_key = *client_key;
  pegasus::Status status = CreateClient(client_impl->address(), factory_method, client_key);
  // Only erase the existing client from the map if creation of the new one succeeded.
  // This helps to ensure the proper accounting of metrics in the presence of
  // re-connection failures (the original client should be released as usual).
  if (status.ok()) {
    // CreateClient() will increment total_clients_metric_ if succeed.
    lock_guard<mutex> lock(client_map_lock_);
    client_map_.erase(client);
  } else {
    // Restore the client used before the failed re-opening attempt, so the caller can
    // properly release it.
    *client_key = old_client_key;
  }
  return status;
}

pegasus::Status ClientCacheHelper::CreateClient(const ClientAddress& address,
    ClientFactory factory_method, ClientKey* client_key) {
  shared_ptr<ClientWrapper> client_impl(factory_method(address, client_key));
  VLOG(2) << "CreateClient(): creating new client for " <<
      ClientAddressToString(client_impl->address());

  // Set the TSocket's send and receive timeouts.
  //client_impl->setRecvTimeout(recv_timeout_ms_);
  //client_impl->setSendTimeout(send_timeout_ms_);
  pegasus::Status status = client_impl->Open(num_tries_, wait_ms_);
  if (!status.ok()) {
    *client_key = nullptr;
    return status;
  }

  // Because the client starts life 'checked out', we don't add it to its host cache.
  {
    lock_guard<mutex> lock(client_map_lock_);
    // Note: MakeClient doesn't set the client_key beause it is only set when open is called.
    *client_key = client_impl->client();
    client_map_[*client_key] = client_impl;
  }

  return pegasus::Status::OK();
}

void ClientCacheHelper::ReleaseClient(ClientKey* client_key) {
  DCHECK(*client_key != NULL) << "Trying to release NULL client";
  shared_ptr<ClientWrapper> client_impl;
  {
    lock_guard<mutex> lock(client_map_lock_);
    ClientMap::iterator client = client_map_.find(*client_key);
    DCHECK(client != client_map_.end());
    client_impl = client->second;
  }
  VLOG(2) << "Releasing client for " << ClientAddressToString(client_impl->address())
      << " back to cache";
  {
    lock_guard<mutex> lock(cache_lock_);
    PerHostCacheMap::iterator cache = per_host_caches_.find(client_impl->address());
    DCHECK(cache != per_host_caches_.end());
    lock_guard<mutex> entry_lock(cache->second->lock);
    cache->second->clients.push_back(*client_key);
  }
  *client_key = NULL;
}

void ClientCacheHelper::DestroyClient(ClientKey* client_key) {
  DCHECK(*client_key != NULL) << "Trying to destroy NULL client";
  shared_ptr<ClientWrapper> client_impl;
  ClientMap::iterator client;
  {
    lock_guard<mutex> lock(client_map_lock_);
    client = client_map_.find(*client_key);
    DCHECK(client != client_map_.end());
    client_impl = client->second;
  }
  VLOG(1) << "Broken Connection, destroy client for " <<
      ClientAddressToString(client_impl->address());

  client_impl->Close();
  lock_guard<mutex> lock(client_map_lock_);
  client_map_.erase(client);
  *client_key = NULL;
}

void ClientCacheHelper::CloseConnections(const ClientAddress& address) {
  PerHostCache* cache;
  {
    lock_guard<mutex> lock(cache_lock_);
    PerHostCacheMap::iterator cache_it = per_host_caches_.find(address);
    if (cache_it == per_host_caches_.end()) return;
    cache = cache_it->second.get();
  }

  {
    VLOG(2) << "Invalidating all " << cache->clients.size() << " clients for: "
            << ClientAddressToString(address);
    lock_guard<mutex> entry_lock(cache->lock);
    lock_guard<mutex> map_lock(client_map_lock_);
    for (ClientKey client_key: cache->clients) {
      ClientMap::iterator client_map_entry = client_map_.find(client_key);
      DCHECK(client_map_entry != client_map_.end());
      client_map_entry->second->Close();
    }
  }
}

string ClientCacheHelper::DebugString() {
  lock_guard<mutex> lock(cache_lock_);
  stringstream out;
  out << "ClientCacheHelper(#hosts=" << per_host_caches_.size()
      << " [";
  bool first = true;
  for (const PerHostCacheMap::value_type& cache: per_host_caches_) {
    lock_guard<mutex> host_cache_lock(cache.second->lock);
    if (!first) out << " ";
    out << ClientAddressToString(cache.first) << ":" << cache.second->clients.size();
    first = false;
  }
  out << "])";
  return out.str();
}

void ClientCacheHelper::TestShutdown() {
  vector<ClientAddress> addresses;
  {
    lock_guard<mutex> lock(cache_lock_);
    for (const PerHostCacheMap::value_type& cache_entry: per_host_caches_) {
      addresses.push_back(cache_entry.first);
    }
  }
  for (const ClientAddress& address: addresses) {
    CloseConnections(address);
  }
}

}