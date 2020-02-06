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

#ifndef PEGASUS_RUNTIME_CLIENT_CACHE_DECLS_H
#define PEGASUS_RUNTIME_CLIENT_CACHE_DECLS_H

namespace pegasus {

/// Forward declarations for client cache types to avoid including the full class
/// declaration indirectly in many places where it is not needed.
template<class T>
class ClientCache;

template<class T>
class ClientConnection;

namespace rpc {
  class FlightClient;
}

/*
/// Common cache / connection types
class ImpalaInternalServiceClient;
typedef ClientCache<ImpalaInternalServiceClient> ImpalaInternalServiceClientCache;
typedef ClientConnection<ImpalaInternalServiceClient> ImpalaInternalServiceConnection;

class CatalogServiceClientWrapper;
typedef ClientCache<CatalogServiceClientWrapper> CatalogServiceClientCache;
typedef ClientConnection<CatalogServiceClientWrapper> CatalogServiceConnection;

class ImpalaBackendClient;
typedef ClientCache<ImpalaBackendClient> ImpalaBackendClientCache;
typedef ClientConnection<ImpalaBackendClient> ImpalaBackendConnection;
*/

}

#endif
