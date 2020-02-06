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

#ifndef PEGASUS_UTIL_TEST_INFO_H
#define PEGASUS_UTIL_TEST_INFO_H

namespace pegasus {

/// Provides global access to whether this binary is running as part of the tests
class TestInfo {
 public:
  enum Mode {
    NON_TEST, // Not a test, one of the main daemons
    TEST,
  };

  /// Called in InitCommonRuntime().
  static void Init(Mode mode) { mode_ = mode; }

  static bool is_test() { return mode_ == TEST; }

 private:
  static Mode mode_;
};

}
#endif
