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

#ifndef PEGASUS_COMMON_INIT_H
#define PEGASUS_COMMON_INIT_H

#include "common/status.h"

// Using the first real-time signal available to initiate graceful shutdown.
// See "Real-time signals" section under signal(7)'s man page for more info.
#define PEGASUS_SHUTDOWN_SIGNAL SIGRTMIN

namespace pegasus {

/// Initialises logging, flags
/// Callers that want to override default gflags variables should do so before calling
/// this method. No logging should be performed until after this method returns.
void InitCommonRuntime(int argc, char** argv);

}

#endif
