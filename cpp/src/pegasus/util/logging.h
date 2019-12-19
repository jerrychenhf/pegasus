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

#ifndef PEGASUS_UTIL_LOGGING_H
#define PEGASUS_UTIL_LOGGING_H

#ifdef GANDIVA_IR

// The LLVM IR code doesn't have an NDEBUG mode. And, it shouldn't include references to
// streams or stdc++. So, making the DCHECK calls void in that case.

#define PEGASUS_IGNORE_EXPR(expr) ((void)(expr))

#define DCHECK(condition) PEGASUS_IGNORE_EXPR(condition)
#define DCHECK_OK(status) PEGASUS_IGNORE_EXPR(status)
#define DCHECK_EQ(val1, val2) PEGASUS_IGNORE_EXPR(val1)
#define DCHECK_NE(val1, val2) PEGASUS_IGNORE_EXPR(val1)
#define DCHECK_LE(val1, val2) PEGASUS_IGNORE_EXPR(val1)
#define DCHECK_LT(val1, val2) PEGASUS_IGNORE_EXPR(val1)
#define DCHECK_GE(val1, val2) PEGASUS_IGNORE_EXPR(val1)
#define DCHECK_GT(val1, val2) PEGASUS_IGNORE_EXPR(val1)

#else  // !GANDIVA_IR

#include <memory>
#include <ostream>
#include <string>

#include "pegasus/util/macros.h"
#include "pegasus/util/visibility.h"

namespace pegasus {
namespace util {

enum class PegasusLogLevel : int {
  PEGASUS_DEBUG = -1,
  PEGASUS_INFO = 0,
  PEGASUS_WARNING = 1,
  PEGASUS_ERROR = 2,
  PEGASUS_FATAL = 3
};

#define PEGASUS_LOG_INTERNAL(level) ::pegasus::util::PegasusLog(__FILE__, __LINE__, level)
#define PEGASUS_LOG(level) PEGASUS_LOG_INTERNAL(::pegasus::util::PegasusLogLevel::PEGASUS_##level)

#define PEGASUS_IGNORE_EXPR(expr) ((void)(expr))

#define PEGASUS_CHECK(condition)                                               \
  PEGASUS_PREDICT_TRUE(condition)                                              \
  ? PEGASUS_IGNORE_EXPR(0)                                                     \
  : ::pegasus::util::Voidify() &                                               \
          ::pegasus::util::PegasusLog(__FILE__, __LINE__,                        \
                                  ::pegasus::util::PegasusLogLevel::PEGASUS_FATAL) \
              << " Check failed: " #condition " "

// If 'to_call' returns a bad status, CHECK immediately with a logged message
// of 'msg' followed by the status.
#define PEGASUS_CHECK_OK_PREPEND(to_call, msg)                                         \
  do {                                                                               \
    ::pegasus::Status _s = (to_call);                                                  \
    PEGASUS_CHECK(_s.ok()) << "Operation failed: " << PEGASUS_STRINGIFY(to_call) << "\n" \
                         << (msg) << ": " << _s.ToString();                          \
  } while (false)

// If the status is bad, CHECK immediately, appending the status to the
// logged message.
#define PEGASUS_CHECK_OK(s) PEGASUS_CHECK_OK_PREPEND(s, "Bad status")

#define PEGASUS_CHECK_EQ(val1, val2) PEGASUS_CHECK((val1) == (val2))
#define PEGASUS_CHECK_NE(val1, val2) PEGASUS_CHECK((val1) != (val2))
#define PEGASUS_CHECK_LE(val1, val2) PEGASUS_CHECK((val1) <= (val2))
#define PEGASUS_CHECK_LT(val1, val2) PEGASUS_CHECK((val1) < (val2))
#define PEGASUS_CHECK_GE(val1, val2) PEGASUS_CHECK((val1) >= (val2))
#define PEGASUS_CHECK_GT(val1, val2) PEGASUS_CHECK((val1) > (val2))

#ifdef NDEBUG
#define PEGASUS_DFATAL ::pegasus::util::PegasusLogLevel::PEGASUS_WARNING

// CAUTION: DCHECK_OK() always evaluates its argument, but other DCHECK*() macros
// only do so in debug mode.

#define DCHECK(condition)                     \
  while (false) PEGASUS_IGNORE_EXPR(condition); \
  while (false) ::pegasus::util::detail::NullLog()
#define DCHECK_OK(s)    \
  PEGASUS_IGNORE_EXPR(s); \
  while (false) ::pegasus::util::detail::NullLog()
#define DCHECK_EQ(val1, val2)            \
  while (false) PEGASUS_IGNORE_EXPR(val1); \
  while (false) PEGASUS_IGNORE_EXPR(val2); \
  while (false) ::pegasus::util::detail::NullLog()
#define DCHECK_NE(val1, val2)            \
  while (false) PEGASUS_IGNORE_EXPR(val1); \
  while (false) PEGASUS_IGNORE_EXPR(val2); \
  while (false) ::pegasus::util::detail::NullLog()
#define DCHECK_LE(val1, val2)            \
  while (false) PEGASUS_IGNORE_EXPR(val1); \
  while (false) PEGASUS_IGNORE_EXPR(val2); \
  while (false) ::pegasus::util::detail::NullLog()
#define DCHECK_LT(val1, val2)            \
  while (false) PEGASUS_IGNORE_EXPR(val1); \
  while (false) PEGASUS_IGNORE_EXPR(val2); \
  while (false) ::pegasus::util::detail::NullLog()
#define DCHECK_GE(val1, val2)            \
  while (false) PEGASUS_IGNORE_EXPR(val1); \
  while (false) PEGASUS_IGNORE_EXPR(val2); \
  while (false) ::pegasus::util::detail::NullLog()
#define DCHECK_GT(val1, val2)            \
  while (false) PEGASUS_IGNORE_EXPR(val1); \
  while (false) PEGASUS_IGNORE_EXPR(val2); \
  while (false) ::pegasus::util::detail::NullLog()

#else
#define PEGASUS_DFATAL ::pegasus::util::PegasusLogLevel::PEGASUS_FATAL

#define DCHECK PEGASUS_CHECK
#define DCHECK_OK PEGASUS_CHECK_OK
#define DCHECK_EQ PEGASUS_CHECK_EQ
#define DCHECK_NE PEGASUS_CHECK_NE
#define DCHECK_LE PEGASUS_CHECK_LE
#define DCHECK_LT PEGASUS_CHECK_LT
#define DCHECK_GE PEGASUS_CHECK_GE
#define DCHECK_GT PEGASUS_CHECK_GT

#endif  // NDEBUG

// This code is adapted from
// https://github.com/ray-project/ray/blob/master/src/ray/util/logging.h.

// To make the logging lib plugable with other logging libs and make
// the implementation unawared by the user, PegasusLog is only a declaration
// which hide the implementation into logging.cc file.
// In logging.cc, we can choose different log libs using different macros.

// This is also a null log which does not output anything.
class PEGASUS_EXPORT PegasusLogBase {
 public:
  virtual ~PegasusLogBase() {}

  virtual bool IsEnabled() const { return false; }

  template <typename T>
  PegasusLogBase& operator<<(const T& t) {
    if (IsEnabled()) {
      Stream() << t;
    }
    return *this;
  }

 protected:
  virtual std::ostream& Stream() = 0;
};

class PEGASUS_EXPORT PegasusLog : public PegasusLogBase {
 public:
  PegasusLog(const char* file_name, int line_number, PegasusLogLevel severity);
  ~PegasusLog() override;

  /// Return whether or not current logging instance is enabled.
  ///
  /// \return True if logging is enabled and false otherwise.
  bool IsEnabled() const override;

  /// The init function of pegasus log for a program which should be called only once.
  ///
  /// \param appName The app name which starts the log.
  /// \param severity_threshold Logging threshold for the program.
  /// \param logDir Logging output file name. If empty, the log won't output to file.
  static void StartPegasusLog(const std::string& appName,
                            PegasusLogLevel severity_threshold = PegasusLogLevel::PEGASUS_INFO,
                            const std::string& logDir = "");

  /// The shutdown function of pegasus log, it should be used with StartPegasusLog as a pair.
  static void ShutDownPegasusLog();

  /// Install the failure signal handler to output call stack when crash.
  /// If glog is not installed, this function won't do anything.
  static void InstallFailureSignalHandler();

  /// Uninstall the signal actions installed by InstallFailureSignalHandler.
  static void UninstallSignalAction();

  /// Return whether or not the log level is enabled in current setting.
  ///
  /// \param log_level The input log level to test.
  /// \return True if input log level is not lower than the threshold.
  static bool IsLevelEnabled(PegasusLogLevel log_level);

 private:
  PEGASUS_DISALLOW_COPY_AND_ASSIGN(PegasusLog);

  // Hide the implementation of log provider by void *.
  // Otherwise, lib user may define the same macro to use the correct header file.
  void* logging_provider_;
  /// True if log messages should be logged and false if they should be ignored.
  bool is_enabled_;

  static PegasusLogLevel severity_threshold_;

 protected:
  std::ostream& Stream() override;
};

// This class make PEGASUS_CHECK compilation pass to change the << operator to void.
// This class is copied from glog.
class PEGASUS_EXPORT Voidify {
 public:
  Voidify() {}
  // This has to be an operator with a precedence lower than << but
  // higher than ?:
  void operator&(PegasusLogBase&) {}
};

namespace detail {

/// @brief A helper for the nil log sink.
///
/// Using this helper is analogous to sending log messages to /dev/null:
/// nothing gets logged.
class NullLog {
 public:
  /// The no-op output operator.
  ///
  /// @param [in] t
  ///   The object to send into the nil sink.
  /// @return Reference to the updated object.
  template <class T>
  NullLog& operator<<(const T& t) {
    return *this;
  }
};

}  // namespace detail
}  // namespace util
}  // namespace pegasus

#endif  // GANDIVA_IR

#endif  // PEGASUS_UTIL_LOGGING_H
