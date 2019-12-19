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


 Status Planner::WokerReport(const ServerCallContext& context, const Action& action,
                  std::unique_ptr<ResultStream>* out) {
    if(action.type == "add") {
      worker_manager
    }

  }

  Status AddWorker(std::unique_ptr<ResultStream>* out) {
    worker_manger.add
    std::shared_ptr<Buffer> buf = Buffer::FromString("ok");
    *out = std::unique_ptr<ResultStream>(new SimpleResultStream({Result{buf}}));
    return Status::OK();
  }
