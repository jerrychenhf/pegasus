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

#include "pegasus/ipc/ipc_server.h"

#include <utility>
#include <errno.h>
#include "pegasus/common/logging.h"
#include "pegasus/ipc/io.h"
#include "pegasus/ipc/fling.h"

namespace pegasus {

#define HANDLE_SIGPIPE(s, fd_)                                              \
  do {                                                                      \
    Status _s = (s);                                                        \
    if (!_s.ok()) {                                                         \
      if (errno == EPIPE || errno == EBADF || errno == ECONNRESET) {        \
        LOG(WARNING)                                                  \
            << "Received SIGPIPE, BAD FILE DESCRIPTOR, or ECONNRESET when " \
               "sending a message to client on fd "                         \
            << fd_                                                          \
            << ". "                                                         \
               "The client on the other end may have hung up.";             \
      } else {                                                              \
        return _s;                                                          \
      }                                                                     \
    }                                                                       \
  } while (0);

int WarnIfSigpipe(int status, int client_sock) {
  if (status >= 0) {
    return 0;
  }
  if (errno == EPIPE || errno == EBADF || errno == ECONNRESET) {
    LOG(WARNING) << "Received SIGPIPE, BAD FILE DESCRIPTOR, or ECONNRESET when "
                          "sending a message to client on fd "
                       << client_sock
                       << ". The client on the other end may "
                          "have hung up.";
    return errno;
  }
  LOG(FATAL) << "Failed to write message to client on fd " << client_sock << ".";
  return -1;  // This is never reached.
}

Client::Client(int fd) : fd(fd) {}
  
IpcServer::IpcServer() {
}


Status IpcServer::Start(const char* socket_name) {
  // Create the event loop.
  loop_.reset(new EventLoop);
  
  int socket = BindIpcSock(socket_name, true);
  
  // TODO(pcm): Check return value.
  CHECK(socket >= 0);

  loop_->AddFileEvent(socket, kEventLoopRead, [this, socket](int events) {
      this->ConnectClient(socket);
    });
  loop_->Start();
  return Status::OK();
}

Status IpcServer::Stop() {
  loop_->Stop();
  return Status::OK();
}

void IpcServer::Shutdown() {
  loop_->Shutdown();
  loop_ = nullptr;
}

IpcServer::~IpcServer() {
  
}

void IpcServer::ConnectClient(int listener_sock) {
  int client_fd = AcceptClient(listener_sock);

  Client* client = new Client(client_fd);
  connected_clients_[client_fd] = std::unique_ptr<Client>(client);

  // Add a callback to handle events on this socket.
  // TODO(pcm): Check return value.
  loop_->AddFileEvent(client_fd, kEventLoopRead, [this, client](int events) {
    Status s = ProcessMessage(client);
    if (!s.ok()) {
      LOG(FATAL) << "Failed to process file event: " << s;
    }
  });
  //LOG(DEBUG) << "New connection with fd " << client_fd;
}

void IpcServer::DisconnectClient(int client_fd) {
   CHECK(client_fd > 0);
  auto it = connected_clients_.find(client_fd);
   CHECK(it != connected_clients_.end());

  loop_->RemoveFileEvent(client_fd);
  // Close the socket.
  close(client_fd);

   LOG(INFO) << "Disconnecting client on fd " << client_fd;
  
  // Release all the objects that the client was using.
  //auto client = it->second.get();
  //TO BE IMPLEMENTED
  
  connected_clients_.erase(it);
}

Status IpcServer::ProcessMessage(Client* client) {
  MessageType type;
  Status s = ReadMessage(client->fd, &type, &input_buffer_);
  CHECK(s.ok() || s.IsIOError());

  uint8_t* input = input_buffer_.data();
  size_t input_size = input_buffer_.size();

  // Process the different types of requests.
  switch (type) {
    case MessageType::GetFileDescriptor: {
      RETURN_IF_ERROR(SendFileDescriptor(client, input, input_size));
    } break;
    case MessageType::DisconnectClient:
      //LOG(DEBUG) << "Disconnecting client on fd " << client->fd;
      DisconnectClient(client->fd);
      break;
    default:
      // This code should be unreachable.
      CHECK(0);
  }
  return Status::OK();
}

Status IpcServer::SendFileDescriptor(Client* client, uint8_t* message, size_t message_size ) {
  // send back all the fds requested by the client
  if(message_size < sizeof(int)) {
    LOG(FATAL) << "Invalid message size: " << message_size;
    return Status::Invalid("Invalid message size");
  }
  int* data = reinterpret_cast<int*>(message);
  int count = *data;
  message_size -= sizeof(int);
  
  if(message_size < sizeof(int) * count) {
    LOG(FATAL) << "Invalid message size: " << message_size;
    return Status::Invalid("Invalid message size");
  }

  int* request_fds = data + 1;
  
  // Send all of the file descriptors for the present objects.
  for (int i = 0; i  < count; i++) {
    int request_fd = *request_fds;
    request_fd++;

    // Only send the file descriptor if it hasn't been sent (see analogous
    // logic in GetStoreFd in client).
    if (client->used_fds.find(request_fd) == client->used_fds.end()) {
      WarnIfSigpipe(send_fd(client->fd, request_fd), client->fd);
      client->used_fds.insert(request_fd);
    } else {
      WarnIfSigpipe(send_fd(client->fd, request_fd), client->fd);
      LOG(WARNING) << "Client request file descriptor which has already been sent. Client fd:  " << client->fd;
    }
  }
  
  return Status::OK();
}

}  // namespace pegasus
