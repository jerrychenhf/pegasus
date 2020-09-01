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

#include "IpcClient.h"

#include <unistd.h>
#include <memory>
#include <cassert>
#include <unordered_set>

#include "ipc/fling.h"
#include "ipc/io.h"

namespace pegasus {

IpcClient::IpcClient() : socket_ (-1) {
}

IpcClient::~IpcClient() {
}
  
bool IpcClient::Connect(const char* socket_name) {
  Status status = ConnectIpcSocketRetry(socket_name, 3, -1, &socket_);
  if (!status.ok())
    return false;
 
  return true;
}

bool IpcClient::Disconnect() {
  if (socket_ != -1) {
    close(socket_);
    socket_ = -1;
  }
  return true;
}

bool IpcClient::MapFileDescriptors(int* mmap_fds, int count) {
  std::unordered_set<int> fds_to_send;
  std::vector<int> server_fds;
  int* fds = mmap_fds;
  for( int i = 0; i < count; i++) {
    int fd = *fds;
    fds++;
    if (fds_to_send.count(fd) == 0 && fd != -1) {
      fds_to_send.insert(fd);
      server_fds.push_back(fd);
    }
  }
  
  if(server_fds.size() > 0) {
    // request the fds for mapping
    return RequestServerFds(server_fds);
  }
  return true;
}

bool IpcClient::RequestServerFds(const std::vector<int>& server_fds) {
  int count = server_fds.size();
  std::vector<int> local_fds;
  local_fds.resize(count);
  
  if (!SendFdRequest(server_fds))
    return false;

  if (!ReceiveFdResponse(local_fds))
    return false;

  // do server fd and local fd mapping
  for (int i = 0; i < count; i++) {
    fd_table_[server_fds[i]] = local_fds[i];
  }

  return true;
}

bool IpcClient::SendFdRequest(const std::vector<int>& server_fds) {
  size_t length = sizeof(int) + sizeof(int) * server_fds.size();
  uint8_t* message = new uint8_t[length];
  int* data = reinterpret_cast<int*>(message);
  *data = server_fds.size();
  memcpy((uint8_t*)(data + 1), (const uint8_t*)server_fds.data(), length - sizeof(int));

  Status status = WriteMessage(socket_, MessageType::GetFileDescriptor, length, message);
  delete[] message;
  if (!status.ok())
    return false;

  return true;
}

bool IpcClient::ReceiveFdResponse(std::vector<int>& local_fds) {
  int count = local_fds.size();
  for (int i = 0; i < count; i++) {
    int local_fd = recv_fd(socket_);
    local_fds[i] = local_fd;
  }
  return true;
}

// If the file descriptor fd has been mmapped in this process before,
// return the pointer that was returned by mmap, otherwise mmap it and store the
// pointer in a hash table.
uint8_t* IpcClient::LookupOrMmap(int fd, int64_t map_size) {
  auto entry = mmap_table_.find(fd);
  if (entry != mmap_table_.end()) {
    return entry->second->pointer();
  } else {
    auto entry_local_fd = fd_table_.find(fd);
    if (entry_local_fd != fd_table_.end()) {
      int local_fd = entry_local_fd->second;
      mmap_table_[fd] =
          std::unique_ptr<MmapTableEntry>(new MmapTableEntry(local_fd, map_size));
      return mmap_table_[fd]->pointer();
    } else {
      return nullptr;
    }
  }
}

// Get a pointer to a file that we know has been memory mapped in this client
// process before.
uint8_t* IpcClient::LookupMmappedFile(int fd) {
  auto entry = mmap_table_.find(fd);
  assert(entry != mmap_table_.end());
  return entry->second->pointer();
}

//To cleanup all the mmap entries, make sure no shared memory pointers are in using
void IpcClient::CleanupMmap() {
   mmap_table_.clear();
}

} // Namespace pegasus