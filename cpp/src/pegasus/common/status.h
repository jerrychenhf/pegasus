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

// Adapted from Apache Arrow

#ifndef PEGASUS_STATUS_H_
#define PEGASUS_STATUS_H_

#include <cstring>
#include <iosfwd>
#include <memory>
#include <string>
#include <utility>
#include "arrow/status.h"
#include "common/compiler-util.h"
#include "common/logging.h"
#include "util/compare.h"
#include "util/macros.h"
#include "util/string_builder.h"
#include "util/visibility.h"

#ifdef PEGASUS_EXTRA_ERROR_CONTEXT

/// \brief Return with given status if condition is met.
#define PEGASUS_RETURN_IF_(condition, status, expr)   \
  do {                                              \
    if (PEGASUS_PREDICT_FALSE(condition)) {           \
      ::pegasus::Status _st = (status);               \
      _st.AddContextLine(__FILE__, __LINE__, expr); \
      return _st;                                   \
    }                                               \
  } while (0)

#else

#define PEGASUS_RETURN_IF_(condition, status, _) \
  do {                                         \
    if (PEGASUS_PREDICT_FALSE(condition)) {      \
      return (status);                         \
    }                                          \
  } while (0)

#endif  // PEGASUS_EXTRA_ERROR_CONTEXT

#define PEGASUS_RETURN_IF(condition, status) \
  PEGASUS_RETURN_IF_(condition, status, PEGASUS_STRINGIFY(status))

/// \brief Propagate any non-successful Status to the caller
#define RETURN_IF_ERROR_STATUS(status)                                   \
  do {                                                                \
    ::pegasus::Status __s = ::pegasus::internal::GenericToStatus(status); \
    PEGASUS_RETURN_IF_(!__s.ok(), __s, PEGASUS_STRINGIFY(status));        \
  } while (false)

#define RETURN_IF_ERROR_ELSE(s, else_)                            \
  do {                                                          \
    ::pegasus::Status _s = ::pegasus::internal::GenericToStatus(s); \
    if (!_s.ok()) {                                             \
      else_;                                                    \
      return _s;                                                \
    }                                                           \
  } while (false)

/// some generally useful macros
#define RETURN_IF_ERROR(stmt)                          \
  do {                                                 \
    const ::pegasus::Status& _status = (stmt);       \
    if (UNLIKELY(!_status.ok())) return _status; \
  } while (false)

#define LOG_AND_RETURN_IF_ERROR(stmt) \
  do { \
    const ::pegasus::Status& _status = (stmt); \
    if (UNLIKELY(!_status.ok()))  { \
      LOG(INFO) << _status.ToString(); \
      return _status; \
    } \
  } while (false)

#define RETURN_VOID_IF_ERROR(stmt)                     \
  do {                                                 \
    if (UNLIKELY(!(stmt).ok())) return;                \
  } while (false)
  
#define ABORT_IF_ERROR(stmt) \
  do { \
    ::pegasus::Status _status = ::pegasus::internal::GenericToStatus(stmt); \
    if (UNLIKELY(!_status.ok())) { \
      ABORT_WITH_ERROR(_status.ToString()); \
    } \
  } while (false)

// Log to FATAL and abort process, generating a core dump if enabled. This should be used
// for unexpected error cases where we want a core dump.
// LOG(FATAL) will call abort().
#define ABORT_WITH_ERROR(msg) \
  do { \
    LOG(FATAL) << msg << ". Service exiting.\n"; \
  } while (false)
  
/// This macro can be appended to a function definition to generate a compiler warning
/// if the result is ignored.
/// TODO: when we upgrade gcc from 4.9.2, we may be able to apply this to the Status
/// type to get this automatically for all Status-returning functions.
#define WARN_UNUSED_RESULT __attribute__((warn_unused_result))
namespace pegasus {

enum class StatusCode : char {
  OK = 0,
  OutOfMemory = 1,
  KeyError = 2,
  TypeError = 3,
  Invalid = 4,
  IOError = 5,
  CapacityError = 6,
  IndexError = 7,
  UnknownError = 9,
  NotImplemented = 10,
  SerializationError = 11,
  ThreadCreationFailed = 12,
  RError = 13,
  ThreadPoolTaskTimeoutError = 14,
  ThreadPoolSubmitFailed = 15,
  RpcTimeout = 16,
  GeneralError = 17,
  ObjectNotFound = 18,
  
  // Gandiva range of errors
  CodeGenError = 40,
  ExpressionValidationError = 41,
  ExecutionError = 42,
  // Continue generic codes.
  AlreadyExists = 45
};

#if defined(__clang__)
// Only clang supports warn_unused_result as a type annotation.
class PEGASUS_MUST_USE_RESULT PEGASUS_EXPORT Status;
#endif

/// \brief An opaque class that allows subsystems to retain
/// additional information inside the Status.
class PEGASUS_EXPORT StatusDetail {
 public:
  virtual ~StatusDetail() = default;
  /// \brief Return a unique id for the type of the StatusDetail
  /// (effectively a poor man's substitude for RTTI).
  virtual const char* type_id() const = 0;
  /// \brief Produce a human-readable description of this status.
  virtual std::string ToString() const = 0;

  bool operator==(const StatusDetail& other) const noexcept {
    return std::string(type_id()) == other.type_id() && ToString() == other.ToString();
  }
};

/// \brief Status outcome object (success or error)
///
/// The Status object is an object holding the outcome of an operation.
/// The outcome is represented as a StatusCode, either success
/// (StatusCode::OK) or an error (any other of the StatusCode enumeration values).
///
/// Additionally, if an error occurred, a specific error message is generally
/// attached.
class PEGASUS_EXPORT Status : public util::EqualityComparable<Status>,
                            public util::ToStringOstreamable<Status> {
 public:
  // Create a success status.
  Status() noexcept : state_(NULLPTR) {}
  ~Status() noexcept {
    // PEGASUS-2400: On certain compilers, splitting off the slow path improves
    // performance significantly.
    if (PEGASUS_PREDICT_FALSE(state_ != NULL)) {
      DeleteState();
    }
  }

  Status(StatusCode code, const std::string& msg);
  /// \brief Pluggable constructor for use by sub-systems.  detail cannot be null.
  Status(StatusCode code, std::string msg, std::shared_ptr<StatusDetail> detail);

  // Copy the specified status.
  inline Status(const Status& s);
  inline Status& operator=(const Status& s);

  // Move the specified status.
  inline Status(Status&& s) noexcept;
  inline Status& operator=(Status&& s) noexcept;

  inline bool Equals(const Status& s) const;

  // AND the statuses.
  inline Status operator&(const Status& s) const noexcept;
  inline Status operator&(Status&& s) const noexcept;
  inline Status& operator&=(const Status& s) noexcept;
  inline Status& operator&=(Status&& s) noexcept;

  /// Return a success status
  static Status OK() { return Status(); }

  template <typename... Args>
  static Status FromArgs(StatusCode code, Args&&... args) {
    return Status(code, util::StringBuilder(std::forward<Args>(args)...));
  }

  template <typename... Args>
  static Status FromDetailAndArgs(StatusCode code, std::shared_ptr<StatusDetail> detail,
                                  Args&&... args) {
    return Status(code, util::StringBuilder(std::forward<Args>(args)...),
                  std::move(detail));
  }

  /// Return an error status for out-of-memory conditions
  template <typename... Args>
  static Status OutOfMemory(Args&&... args) {
    return Status::FromArgs(StatusCode::OutOfMemory, std::forward<Args>(args)...);
  }

  /// Return an error status for failed key lookups (e.g. column name in a table)
  template <typename... Args>
  static Status KeyError(Args&&... args) {
    return Status::FromArgs(StatusCode::KeyError, std::forward<Args>(args)...);
  }

  /// Return an error status for type errors (such as mismatching data types)
  template <typename... Args>
  static Status TypeError(Args&&... args) {
    return Status::FromArgs(StatusCode::TypeError, std::forward<Args>(args)...);
  }

  /// Return an error status for unknown errors
  template <typename... Args>
  static Status UnknownError(Args&&... args) {
    return Status::FromArgs(StatusCode::UnknownError, std::forward<Args>(args)...);
  }

  /// Return an error status when an operation or a combination of operation and
  /// data types is unimplemented
  template <typename... Args>
  static Status NotImplemented(Args&&... args) {
    return Status::FromArgs(StatusCode::NotImplemented, std::forward<Args>(args)...);
  }

  /// Return an error status for invalid data (for example a string that fails parsing)
  template <typename... Args>
  static Status Invalid(Args&&... args) {
    return Status::FromArgs(StatusCode::Invalid, std::forward<Args>(args)...);
  }

  /// Return an error status when an index is out of bounds
  template <typename... Args>
  static Status IndexError(Args&&... args) {
    return Status::FromArgs(StatusCode::IndexError, std::forward<Args>(args)...);
  }

  /// Return an error status when a container's capacity would exceed its limits
  template <typename... Args>
  static Status CapacityError(Args&&... args) {
    return Status::FromArgs(StatusCode::CapacityError, std::forward<Args>(args)...);
  }

  /// Return an error status when some IO-related operation failed
  template <typename... Args>
  static Status IOError(Args&&... args) {
    return Status::FromArgs(StatusCode::IOError, std::forward<Args>(args)...);
  }

  /// Return an error status when some (de)serialization operation failed
  template <typename... Args>
  static Status SerializationError(Args&&... args) {
    return Status::FromArgs(StatusCode::SerializationError, std::forward<Args>(args)...);
  }
  
  template <typename... Args>
  static Status ThreadCreationFailed(Args&&... args) {
    return Status::FromArgs(StatusCode::ThreadCreationFailed, std::forward<Args>(args)...);
  }
  
  template <typename... Args>
  static Status RError(Args&&... args) {
    return Status::FromArgs(StatusCode::RError, std::forward<Args>(args)...);
  }
  
  template <typename... Args>
  static Status ThreadPoolTaskTimeoutError(Args&&... args) {
    return Status::FromArgs(StatusCode::ThreadPoolTaskTimeoutError, std::forward<Args>(args)...);
  }
  
  template <typename... Args>
  static Status ThreadPoolSubmitFailed(Args&&... args) {
    return Status::FromArgs(StatusCode::ThreadPoolSubmitFailed, std::forward<Args>(args)...);
  }
  
  template <typename... Args>
  static Status GeneralError(Args&&... args) {
    return Status::FromArgs(StatusCode::GeneralError, std::forward<Args>(args)...);
  }
  
  template <typename... Args>
  static Status ObjectNotFound(Args&&... args) {
    return Status::FromArgs(StatusCode::ObjectNotFound, std::forward<Args>(args)...);
  }

  template <typename... Args>
  static Status CodeGenError(Args&&... args) {
    return Status::FromArgs(StatusCode::CodeGenError, std::forward<Args>(args)...);
  }

  template <typename... Args>
  static Status ExpressionValidationError(Args&&... args) {
    return Status::FromArgs(StatusCode::ExpressionValidationError,
                            std::forward<Args>(args)...);
  }

  template <typename... Args>
  static Status ExecutionError(Args&&... args) {
    return Status::FromArgs(StatusCode::ExecutionError, std::forward<Args>(args)...);
  }

  template <typename... Args>
  static Status AlreadyExists(Args&&... args) {
    return Status::FromArgs(StatusCode::AlreadyExists, std::forward<Args>(args)...);
  }

  /// Return true iff the status indicates success.
  bool ok() const { return (state_ == NULLPTR); }

  /// Return true iff the status indicates an out-of-memory error.
  bool IsOutOfMemory() const { return code() == StatusCode::OutOfMemory; }
  /// Return true iff the status indicates a key lookup error.
  bool IsKeyError() const { return code() == StatusCode::KeyError; }
  /// Return true iff the status indicates invalid data.
  bool IsInvalid() const { return code() == StatusCode::Invalid; }
  /// Return true iff the status indicates an IO-related failure.
  bool IsIOError() const { return code() == StatusCode::IOError; }
  /// Return true iff the status indicates a container reaching capacity limits.
  bool IsCapacityError() const { return code() == StatusCode::CapacityError; }
  /// Return true iff the status indicates an out of bounds index.
  bool IsIndexError() const { return code() == StatusCode::IndexError; }
  /// Return true iff the status indicates a type error.
  bool IsTypeError() const { return code() == StatusCode::TypeError; }
  /// Return true iff the status indicates an unknown error.
  bool IsUnknownError() const { return code() == StatusCode::UnknownError; }
  /// Return true iff the status indicates an unimplemented operation.
  bool IsNotImplemented() const { return code() == StatusCode::NotImplemented; }
  /// Return true iff the status indicates a (de)serialization failure
  bool IsSerializationError() const { return code() == StatusCode::SerializationError; }
  /// Return true iff the status indicates a R-originated error.
  bool IsRError() const { return code() == StatusCode::RError; }

  bool IsCodeGenError() const { return code() == StatusCode::CodeGenError; }

  bool IsExpressionValidationError() const {
    return code() == StatusCode::ExpressionValidationError;
  }

  bool IsExecutionError() const { return code() == StatusCode::ExecutionError; }
  
  arrow::Status toArrowStatus() {
    if (ok()){
      return arrow::Status::OK();
    }
    
    // TO DO
    // translate other error codes to Arrow Status
    return arrow::Status::UnknownError(message());
  }
  
  static Status fromArrowStatus(arrow::Status status) {
    if (status.ok()){
      return Status::OK();
    }
    
    // TO DO
    // translate other error codes to Status
    return Status::UnknownError(status.message());
  }

  /// \brief Return a string representation of this status suitable for printing.
  ///
  /// The string "OK" is returned for success.
  std::string ToString() const;

  /// \brief Return a string representation of the status code, without the message
  /// text or POSIX code information.
  std::string CodeAsString() const;

  /// \brief Return the StatusCode value attached to this status.
  StatusCode code() const { return ok() ? StatusCode::OK : state_->code; }

  /// \brief Return the specific error message attached to this status.
  std::string message() const { return ok() ? "" : state_->msg; }

  /// \brief Return the status detail attached to this message.
  std::shared_ptr<StatusDetail> detail() const {
    return state_ == NULLPTR ? NULLPTR : state_->detail;
  }

  /// \brief Return a new Status copying the existing status, but
  /// updating with the existing detail.
  Status WithDetail(std::shared_ptr<StatusDetail> new_detail) const {
    return Status(code(), message(), std::move(new_detail));
  }

  /// \brief Return a new Status with changed message, copying the
  /// existing status code and detail.
  Status WithMessage(std::string message) const {
    return Status(code(), std::move(message), detail());
  }

  [[noreturn]] void Abort() const;
  [[noreturn]] void Abort(const std::string& message) const;

#ifdef PEGASUS_EXTRA_ERROR_CONTEXT
  void AddContextLine(const char* filename, int line, const char* expr);
#endif

 private:
  struct State {
    StatusCode code;
    std::string msg;
    std::shared_ptr<StatusDetail> detail;
  };
  // OK status has a `NULL` state_.  Otherwise, `state_` points to
  // a `State` structure containing the error code and message(s)
  State* state_;

  void DeleteState() {
    delete state_;
    state_ = NULLPTR;
  }
  void CopyFrom(const Status& s);
  inline void MoveFrom(Status& s);
};

void Status::MoveFrom(Status& s) {
  delete state_;
  state_ = s.state_;
  s.state_ = NULLPTR;
}

Status::Status(const Status& s)
    : state_((s.state_ == NULLPTR) ? NULLPTR : new State(*s.state_)) {}

Status& Status::operator=(const Status& s) {
  // The following condition catches both aliasing (when this == &s),
  // and the common case where both s and *this are ok.
  if (state_ != s.state_) {
    CopyFrom(s);
  }
  return *this;
}

Status::Status(Status&& s) noexcept : state_(s.state_) { s.state_ = NULLPTR; }

Status& Status::operator=(Status&& s) noexcept {
  MoveFrom(s);
  return *this;
}

bool Status::Equals(const Status& s) const {
  if (state_ == s.state_) {
    return true;
  }

  if (ok() || s.ok()) {
    return false;
  }

  if (detail() != s.detail() && !(*detail() == *s.detail())) {
    return false;
  }

  return code() == s.code() && message() == s.message();
}

/// \cond FALSE
// (note: emits warnings on Doxygen < 1.8.15,
//  see https://github.com/doxygen/doxygen/issues/6295)
Status Status::operator&(const Status& s) const noexcept {
  if (ok()) {
    return s;
  } else {
    return *this;
  }
}

Status Status::operator&(Status&& s) const noexcept {
  if (ok()) {
    return std::move(s);
  } else {
    return *this;
  }
}

Status& Status::operator&=(const Status& s) noexcept {
  if (ok() && !s.ok()) {
    CopyFrom(s);
  }
  return *this;
}

Status& Status::operator&=(Status&& s) noexcept {
  if (ok() && !s.ok()) {
    MoveFrom(s);
  }
  return *this;
}
/// \endcond

namespace internal {

// Extract Status from Status or Result<T>
// Useful for the status check macros such as RETURN_NOT_OK.
inline Status GenericToStatus(const Status& st) { return st; }

}  // namespace internal

}  // namespace pegasus

#endif  // PEGASUS_STATUS_H_

