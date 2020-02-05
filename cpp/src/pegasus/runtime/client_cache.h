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

#ifndef PAGASUS_RUNTIME_CLIENT_CACHE_H
#define PAGASUS_RUNTIME_CLIENT_CACHE_H

#include <vector>
#include <list>
#include <string>
#include <boost/unordered_map.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>

#include "util/network-util.h"
#include "util/time.h"

#include "common/status.h"

namespace pegasus {
  
namespace rpc {
   class FlightClient;
}

  
/// Opaque pointer type which allows users of ClientCache to refer to particular client
/// instances without requiring that we parameterise ClientCacheHelper by type.
typedef void* ClientKey;

/// The client address presented currently in string
typedef std::string ClientAddress;

#define ClientAddressToString(address) address

class ClientWrapper {
 public:
  ClientWrapper(ClientAddress addr) : address_(addr) {
  }
  
  ClientAddress address() {
    return address_;
  }
  
  Status Close() {
    //TO DO
    return Status::OK();
  }
  
  Status Open(uint32_t num_tries, uint64_t wait_ms);
private:
  ClientAddress address_;
  std::unique_ptr<rpc::FlightClient> client_;
};

/// Helper class which implements the majority of the caching functionality without using
/// templates (i.e. pointers to the superclass of all ThriftClients and a void* for the
/// key). This class is for internal use only; the public interface is in ClientCache
/// below.
//
/// A client is either 'in-use' (the user of the cache is between GetClient() and
/// ReleaseClient() pairs) or 'cached', in which case it is available for the next
/// GetClient() call. Internally, this class maintains a map of all clients, in use or not,
/// which is indexed by their ClientKey (see below), and a map from server address to a
/// list of the keys of all clients that are not currently in use.
//
/// The user of this class only sees RPC proxy classes, but we have to track the
/// ThriftClient to manipulate the underlying transport. To do this, we use an opaque
/// ClientKey pointer type to act as the key for a particular client. We actually know the
/// type of the value at the end of pointer (it's the type parameter to ClientCache), but
/// we deliberately avoid using it so that we don't have to parameterise this class by
/// type, and thus this entire class doesn't get inlined every time it gets used.
//
/// This class is thread-safe.
//
/// TODO: shut down clients in the background if they don't get used for a period of time
/// TODO: More graceful handling of clients that have failed (maybe better
/// handled by a smart-wrapper of the interface object).
/// TODO: limits on total number of clients, and clients per-backend
/// TODO: move this to a separate header file, so that the public interface is more
/// prominent in this file
class ClientCacheHelper {
 public:
  /// Callback method which produces a client object when one cannot be found in the
  /// cache. Supplied by the ClientCache wrapper.
  typedef boost::function<ClientWrapper* (const ClientAddress& address,
                                             ClientKey* client_key)> ClientFactory;

  /// Returns a client for the given address in 'client_key'. If a previously created
  /// client is not available (i.e. there are no entries in the per-host cache), a new
  /// client is created by calling the supplied 'factory_method'. As a postcondition, the
  /// returned client will not be present in the per-host cache.
  //
  /// If there is an error creating the new client, *client_key will be NULL.
  pegasus::Status GetClient(const ClientAddress& address, ClientFactory factory_method,
      ClientKey* client_key) WARN_UNUSED_RESULT;

  /// Returns a newly-opened client in client_key. May reopen the existing client, or may
  /// replace it with a new one (created using 'factory_method').
  //
  /// Returns an error status and sets 'client_key' to NULL if a new client cannot
  /// created.
  pegasus::Status ReopenClient(
      ClientFactory factory_method, ClientKey* client_key) WARN_UNUSED_RESULT;

  /// Returns a client to the cache. Upon return, *client_key will be NULL, and the
  /// associated client will be available in the per-host cache.
  void ReleaseClient(ClientKey* client_key);

  /// Close all connections to a host (e.g., in case of failure) so that on their
  /// next use they will have to be reopened via ReopenClient().
  void CloseConnections(const ClientAddress& address);

  /// Close the client connection and don't put client back to per-host cache.
  /// Also remove client from client_map_.
  void DestroyClient(ClientKey* client_key);

  /// Return a debug representation of the contents of this cache.
  std::string DebugString();

  /// Closes every connection in the cache. Used only for testing.
  void TestShutdown();

 private:
  template <class T> friend class ClientCache;
  /// Private constructor so that only ClientCache can instantiate this class.
  ClientCacheHelper(uint32_t num_tries, uint64_t wait_ms, int32_t send_timeout_ms,
      int32_t recv_timeout_ms)
      : num_tries_(num_tries),
        wait_ms_(wait_ms),
        send_timeout_ms_(send_timeout_ms),
        recv_timeout_ms_(recv_timeout_ms){ }

  /// There are three lock categories - the cache-wide lock (cache_lock_), the locks for a
  /// specific cache (PerHostCache::lock) and the lock for the set of all clients
  /// (client_map_lock_). They do not have to be taken concurrently (and should not be, in
  /// general), but if they are they must be taken in this order:
  /// cache_lock_->PerHostCache::lock->client_map_lock_.

  /// A PerHostCache is a list of available client keys for a single host, plus a lock that
  /// protects that list. Only one PerHostCache will ever be created for a given host, so
  /// when a PerHostCache is retrieved from the PerHostCacheMap containing it there is no
  /// need to hold on to the container's lock.
  //
  /// Only clients that are not currently in use are tracked in their host's
  /// PerHostCache. When a client is returned to the cache via ReleaseClient(), it is
  /// reinserted into its corresponding PerHostCache list. Clients returned by GetClient()
  /// are considered to be immediately in use, and so don't exist in their PerHostCache
  /// until they are released for the first time.
  struct PerHostCache {
    /// Protects clients.
    boost::mutex lock;

    /// List of client keys for this entry's host.
    std::list<ClientKey> clients;
  };

  /// Protects per_host_caches_
  boost::mutex cache_lock_;

  /// Map from an address to a PerHostCache containing a list of keys that have entries in
  /// client_map_ for that host. The value type is wrapped in a shared_ptr so that the
  /// copy c'tor for PerHostCache is not required.
  typedef boost::unordered_map<
      ClientAddress, std::shared_ptr<PerHostCache>> PerHostCacheMap;
  PerHostCacheMap per_host_caches_;

  /// Protects client_map_.
  boost::mutex client_map_lock_;

  /// Map from client key back to its associated ThriftClientImpl transport. This is where
  /// all the clients are actually stored, and client instances are owned by this class
  /// and persist for exactly as long as they are present in this map.
  /// We use a map (vs. unordered_map) so we get iterator consistency across operations.
  typedef std::map<ClientKey, std::shared_ptr<ClientWrapper>> ClientMap;
  ClientMap client_map_;

  /// Number of attempts to make to open a connection. 0 means retry indefinitely.
  const uint32_t num_tries_;

  /// Time to wait between failed connection attempts.
  const uint64_t wait_ms_;

  /// Time to wait for the underlying socket to send data, e.g., for an RPC.
  const int32_t send_timeout_ms_;

  /// Time to wait for the underlying socket to receive data, e.g., for an RPC response.
  const int32_t recv_timeout_ms_;

  /// Create a new client for specific address in 'client' and put it in client_map_
  pegasus::Status CreateClient(const ClientAddress& address, ClientFactory factory_method,
      ClientKey* client_key) WARN_UNUSED_RESULT;
};

}

#endif