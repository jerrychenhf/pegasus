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

#include "server/worker/ipc_server_thread.h"

#include <string>
#include <signal.h>
#include "util/global_flags.h"
#include "util/time.h"
#include "common/logging.h"

using namespace boost;

DECLARE_string(ipc_socket_name);

namespace pegasus {

IpcServerThread::IpcServerThread() {
  ipc_server_ = nullptr;
}

IpcServerThread::~IpcServerThread() {
  
}

Status IpcServerThread::Init() {
  LOG(INFO) << "Ipc Server initialized.";
  ipc_server_.reset(new IpcServer());
  return Status::OK();
}

Status IpcServerThread::Start() {
  LOG(INFO) << "Starting Ipc Server listening on " << FLAGS_ipc_socket_name;

  Status status = Thread::Create("ipc-server", "ipc-server-main",
          boost::bind<void>(boost::mem_fn(&IpcServerThread::IpcMainThread), this), &thread_,
          false);
  if (!status.ok()) {
    return status;
  }
  
  LOG(INFO) << "Ipc Server started on socket " << FLAGS_ipc_socket_name;
  return Status::OK();
}

Status IpcServerThread::Stop() {
  //shutdown
  // TO DO
  
  thread_->Join();
  return Status::OK();
}

  /// Driver method for the IpcServer
  /// until it is shutdown.
void IpcServerThread::IpcMainThread() {
  Status status = StartServer(FLAGS_ipc_socket_name.c_str());
  if (!status.ok()) {
    LOG(FATAL) << "Ipc Server failed to start on socket " << FLAGS_ipc_socket_name;
  } else {
    LOG(INFO) << "Ipc Server started on socket " << FLAGS_ipc_socket_name;
  }
}
  
Status IpcServerThread::StartServer(const char* socket_name) {
  // Ignore SIGPIPE signals. If we don't do this, then when we attempt to write
  // to a client that has already died, the store could die.
  signal(SIGPIPE, SIG_IGN);
  
  // start will run the event loop until Stop called
  ipc_server_->Start(socket_name);
  
  // when it comes here, the stop is called and the even loop exit
  ipc_server_->Shutdown();
  
   return Status::OK();
}

Status IpcServerThread::StopServer() {
  // TO BE CHECKED
  // whether this can be called in the main thread?
  if (ipc_server_ != nullptr) {
      ipc_server_->Stop();
  }
  
  return Status::OK();
}

} // namespace pegasus