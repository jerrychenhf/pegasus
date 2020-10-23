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

#ifndef PEGASUS_IPC_SERVER_H
#define PEGASUS_IPC_SERVER_H

#include <functional>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "pegasus/common/status.h"
#include "pegasus/ipc/events.h"

namespace pegasus {

int WarnIfSigpipe(int status, int client_sock);

/// Contains all information that is associated with a Plasma store client.
struct Client {
  explicit Client(int fd);

  /// The file descriptor used to communicate with the client.
  int fd;
  
  /// File descriptors that are used by this client.
  std::unordered_set<int> used_fds;

  std::string name = "anonymous_client";
};

class IpcServer {
 public:
  IpcServer();

  ~IpcServer();

  /// \brief Run the server
  Status Start(const char* socket_name);

  /// \brief Stop the server
  Status Stop();

  void Shutdown();

  /// Connect a new client to the PlasmaStore.
  ///
  /// \param listener_sock The socket that is listening to incoming connections.
  void ConnectClient(int listener_sock);

  /// Disconnect a client from the PlasmaStore.
  ///
  /// \param client_fd The client file descriptor that is disconnected.
  void DisconnectClient(int client_fd);
  
  Status ProcessMessage(Client* client);

  Status SendFileDescriptor(Client* client, uint8_t* message, size_t message_size );
 private:
  std::unique_ptr<EventLoop> loop_;
    
   /// Input buffer. This is allocated only once to avoid mallocs for every
  /// call to process_message.
  std::vector<uint8_t> input_buffer_;
    
  std::unordered_map<int, std::unique_ptr<Client>> connected_clients_;
};
  
}  // namespace pegasus

#endif //PEGASUS_IPC_SERVER_H