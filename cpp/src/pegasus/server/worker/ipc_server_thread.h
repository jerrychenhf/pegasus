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

#ifndef PEGASUS_IPC_SERVER_THREAD_H
#define PEGASUS_IPC_SERVER_THREAD_H

#include "common/status.h"
#include "util/thread.h"
#include "ipc/ipc_server.h"

using namespace std;

namespace pegasus {

template <typename T>
class ThreadPool;

class IpcServerThread {
 public:
  IpcServerThread();
  ~IpcServerThread();
  
  Status Init();

  Status Start();
  Status Stop();
  
 private:
  std::unique_ptr<Thread> thread_;
  std::unique_ptr<IpcServer> ipc_server_;

  void IpcMainThread();

  Status StartServer(const char* socket_name) ;
  Status StopServer();

};

} // namespace pegasus

#endif  // PEGASUS_IPC_SERVER_THREAD_H
