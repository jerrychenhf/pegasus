//
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

// This is a generated file, DO NOT EDIT IT.
// To change this file, see impala/bin/gen_build_version.py

#include "common/version.h"
#include "common/config.h"

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define PEGASUS_BUILD_VERSION STR(PEGASUS_VERSION_MAJOR) "." STR(PEGASUS_VERSION_MINOR) "." STR(PEGASUS_VERSION_PATCH)
#define PEGASUS_BUILD_HASH "8dcff3aa41e7f252aa27c6ab1275712103ed5d2c"
#define PEGASUS_BUILD_TIME "Fri Mar  9 06:41:18 CST 2018"

const char* GetDaemonBuildVersion() {
  return PEGASUS_BUILD_VERSION;
}

const char* GetDaemonBuildHash() {
  return PEGASUS_BUILD_HASH;
}

const char* GetDaemonBuildTime() {
  return PEGASUS_BUILD_TIME;
}