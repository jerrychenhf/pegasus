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

#include "util/error-util.h"

#include <errno.h>
#include <string.h>
#include <sstream>

#include "common/names.h"

namespace pegasus {

string GetStrErrMsg() {
  // Save errno. "<<" could reset it.
  int e = errno;
  return GetStrErrMsg(e);
}

string GetStrErrMsg(int err_no) {
  if (err_no == 0) return "";
  stringstream ss;
  char buf[1024];
  ss << "Error(" << err_no << "): " << strerror_r(err_no, buf, 1024);
  return ss.str();
}


}
