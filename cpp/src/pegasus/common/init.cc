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

#include "common/init.h"

#include "util/cpu-info.h"
#include "util/disk-info.h"
#include "util/mem-info.h"
#include "util/os-info.h"
#include "util/test-info.h"
#include "util/network-util.h"
#include "util/thread.h"
#include "util/debug-util.h"

using namespace pegasus;

DECLARE_string(hostname);

#ifdef CODE_COVERAGE_ENABLED
extern "C" { void __gcov_flush(); }
#endif

void pegasus::InitCommonRuntime(int argc, char** argv,
  TestInfo::Mode test_mode) {
  // TO DO
  // do common initialize tasks such as logging and flags
  CpuInfo::Init();
  DiskInfo::Init();
  MemInfo::Init();
  OsInfo::Init();
  TestInfo::Init(test_mode);
  
  // Verify CPU meets the minimum requirements before calling InitGoogleLoggingSafe()
  // which might use SSSE3 instructions (see IMPALA-160).
  CpuInfo::VerifyCpuRequirements();
  
  // Set the default hostname. The user can override this with the hostname flag.
  ABORT_IF_ERROR(GetHostname(&FLAGS_hostname));
  
  google::SetVersionString(pegasus::GetBuildVersion());
  google::ParseCommandLineFlags(&argc, &argv, true);
  
  pegasus::InitGoogleLoggingSafe(argv[0]);
  
  pegasus::InitThreading();
  
  LOG(INFO) << "Using hostname: " << FLAGS_hostname;
  pegasus::LogCommandLineFlags();
  
  LOG(INFO) << CpuInfo::DebugString();
  LOG(INFO) << DiskInfo::DebugString();
  LOG(INFO) << MemInfo::DebugString();
  LOG(INFO) << OsInfo::DebugString();
  LOG(INFO) << "Process ID: " << getpid();
}

#if defined(ADDRESS_SANITIZER)
// Default ASAN_OPTIONS. Override by setting environment variable $ASAN_OPTIONS.
extern "C" const char *__asan_default_options() {
  // IMPALA-2746: backend tests don't pass with leak sanitizer enabled.
  return "handle_segv=0 detect_leaks=0 allocator_may_return_null=1";
}
#endif

#if defined(THREAD_SANITIZER)
// Default TSAN_OPTIONS. Override by setting environment variable $TSAN_OPTIONS.
extern "C" const char *__tsan_default_options() {
  // Note that backend test should re-configure to halt_on_error=1
  return "halt_on_error=0 history_size=7";
}
#endif

// Default UBSAN_OPTIONS. Override by setting environment variable $UBSAN_OPTIONS.
#if defined(UNDEFINED_SANITIZER)
extern "C" const char *__ubsan_default_options() {
  return "print_stacktrace=1 suppressions=" UNDEFINED_SANITIZER_SUPPRESSIONS;
}
#endif
