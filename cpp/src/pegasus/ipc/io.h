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

#ifndef PEGASUS_IPC_IO_H
#define PEGASUS_IPC_IO_H

#include <inttypes.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <memory>
#include <string>
#include <vector>

#include "common/status.h"

namespace pegasus {

enum class MessageType : int32_t {
  DisconnectClient = 0,
  GetFileDescriptor = 1,
  MIN = DisconnectClient,
  MAX = GetFileDescriptor
};

constexpr int64_t kProtocolVersion = 0x0000000000000001;

using pegasus::Status;

Status WriteBytes(int fd, uint8_t* cursor, size_t length);

Status ReadBytes(int fd, uint8_t* cursor, size_t length);

int BindIpcSock(const std::string& pathname, bool shall_listen);

int ConnectIpcSock(const std::string& pathname);

Status ConnectIpcSocketRetry(const std::string& pathname, int num_retries,
                             int64_t timeout, int* fd);

int AcceptClient(int socket_fd);

std::unique_ptr<uint8_t[]> ReadMessageAsync(int sock);
  
Status WriteMessage(int fd, MessageType type, int64_t length, uint8_t* bytes);

Status ReadMessage(int fd, MessageType* type, std::vector<uint8_t>* buffer);

}  // namespace pegasus

#endif //PEGASUS_IPC_IO_H