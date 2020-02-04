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

#include "util/network-util.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <limits.h>
#include <algorithm>
#include <sstream>
#include <random>
#include <vector>
#include <boost/algorithm/string.hpp>

#include "util/error-util.h"
//#include "exec/kudu-util.h"
//#include "kudu/util/net/sockaddr.h"
//#include <util/string-parser.h>

#include "common/names.h"

using boost::algorithm::is_any_of;
using boost::algorithm::split;
using std::find;
using std::random_device;

#ifdef __APPLE__
// OS X does not seem to have a similar limitation as Linux and thus the
// macro is not defined.
#define HOST_NAME_MAX 64
#endif

namespace pegasus {

const string LOCALHOST_IP_STR("127.0.0.1");

Status GetHostname(string* hostname) {
  char name[HOST_NAME_MAX];
  int ret = gethostname(name, HOST_NAME_MAX);
  if (ret != 0) {
    string error_msg = GetStrErrMsg();
    stringstream ss;
    ss << "Could not get hostname: " << error_msg;
    return Status::GeneralError(ss.str());
  }
  *hostname = string(name);
  return Status::OK();
}

Status HostnameToIpAddr(const Hostname& hostname, IpAddr* ip){
  // Try to resolve via the operating system.
  vector<IpAddr> addresses;
  addrinfo hints;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET; // IPv4 addresses only
  hints.ai_socktype = SOCK_STREAM;

  struct addrinfo* addr_info;
  if (getaddrinfo(hostname.c_str(), NULL, &hints, &addr_info) != 0) {
    stringstream ss;
    ss << "Could not find IPv4 address for: " << hostname;
    return Status::GeneralError(ss.str());
  }

  addrinfo* it = addr_info;
  while (it != NULL) {
    char addr_buf[64];
    const char* result =
        inet_ntop(AF_INET, &((sockaddr_in*)it->ai_addr)->sin_addr, addr_buf, 64);
    if (result == NULL) {
      stringstream ss;
      ss << "Could not convert IPv4 address for: " << hostname;
      freeaddrinfo(addr_info);
      return Status::GeneralError(ss.str());
    }
    addresses.push_back(string(addr_buf));
    it = it->ai_next;
  }

  freeaddrinfo(addr_info);

  if (addresses.empty()) {
    stringstream ss;
    ss << "Could not convert IPv4 address for: " << hostname;
    return Status::GeneralError(ss.str());
  }

  // RFC 3484 only specifies a partial order for the result of getaddrinfo() so we need to
  // sort the addresses before picking the first non-localhost one.
  sort(addresses.begin(), addresses.end());

  // Try to find a non-localhost address, otherwise just use the first IP address
  // returned.
  *ip = addresses[0];
  if (!FindFirstNonLocalhost(addresses, ip)) {
    VLOG(3) << "Only localhost addresses found for " << hostname;
  }
  return Status::OK();
}

bool FindFirstNonLocalhost(const vector<string>& addresses, string* addr) {
  for (const string& candidate: addresses) {
    if (candidate != LOCALHOST_IP_STR) {
      *addr = candidate;
      return true;
    }
  }

  return false;
}

bool IsWildcardAddress(const string& ipaddress) {
  return ipaddress == "0.0.0.0";
}

/// Pick a random port in the range of ephemeral ports
/// https://tools.ietf.org/html/rfc6335
int FindUnusedEphemeralPort() {
  static uint32_t LOWER = 49152, UPPER = 65000;
  random_device rd;
  srand(rd());

  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) return -1;
  struct sockaddr_in server_address;
  bzero(reinterpret_cast<char*>(&server_address), sizeof(server_address));
  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = INADDR_ANY;
  for (int tries = 0; tries < 100; ++tries) {
    int port = LOWER + rand() % (UPPER - LOWER);
    server_address.sin_port = htons(port);
    if (bind(sockfd, reinterpret_cast<struct sockaddr*>(&server_address),
        sizeof(server_address)) == 0) {
      close(sockfd);
      return port;
    }
  }
  close(sockfd);
  return -1;
}

}
