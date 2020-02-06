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

#include "util/debug-util.h"

#include <iomanip>
#include <random>
#include <sstream>
#include <boost/algorithm/string.hpp>
#include <gflags/gflags.h>

#include "common/version.h"
#include "gutil/strings/substitute.h"

// / WARNING this uses a private API of GLog: DumpStackTraceToString().
namespace google {
namespace glog_internal_namespace_ {
extern void DumpStackTraceToString(std::string* s);
}
}

#include "common/names.h"

DECLARE_string(hostname);
DECLARE_int32(planner_port);
DECLARE_int32(worker_port);

namespace pegasus {

string GetBuildVersion(bool compact) {
  stringstream ss;
  ss << GetDaemonBuildVersion()
#ifdef NDEBUG
     << " RELEASE"
#else
     << " DEBUG"
#endif
     << " (build " << GetDaemonBuildHash()
     << ")";
  if (!compact) {
    ss << endl << "Built on " << GetDaemonBuildTime();
  }
  return ss.str();
}

string GetVersionString(bool compact) {
  stringstream ss;
  ss << google::ProgramInvocationShortName()
     << " version " << GetBuildVersion(compact);
  return ss.str();
}

string GetStackTrace() {
  string s;
  google::glog_internal_namespace_::DumpStackTraceToString(&s);
  return s;
}

string GetWorkerString() {
  return Substitute("$0:$1", FLAGS_hostname, FLAGS_worker_port);
}

string GetPlannerString() {
  return Substitute("$0:$1", FLAGS_hostname, FLAGS_planner_port);
}

}
