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

#ifndef PEGASUS_COMMON_LOGGING_H
#define PEGASUS_COMMON_LOGGING_H

#include <glog/logging.h>
#include <gflags/gflags.h>

/// Define a wrapper around DCHECK for strongly typed enums that print a useful error
/// message on failure.
#define DCHECK_ENUM_EQ(a, b)                                               \
  DCHECK(a == b) << "[ " #a " = " << static_cast<int>(a) << " , " #b " = " \
                 << static_cast<int>(b) << " ]"

/// IR modules don't use these methods, and can't see the google namespace used in
/// GetFullLogFilename()'s prototype.
namespace pegasus {

/// glog doesn't allow multiple invocations of InitGoogleLogging(). This method
/// conditionally calls InitGoogleLogging() only if it hasn't been called before.
void InitGoogleLoggingSafe(const char* arg);

/// Returns the full pathname of the symlink to the most recent log
/// file corresponding to this severity
void GetFullLogFilename(google::LogSeverity severity, std::string* filename);

/// Shuts down the google logging library. Call before exit to ensure that log files are
/// flushed. May only be called once.
void ShutdownLogging();

/// Writes all command-line flags to the log at level INFO.
void LogCommandLineFlags();
}

#endif
