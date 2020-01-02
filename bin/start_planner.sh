#!/bin/sh

# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.
# The ASF licenses this file to You under the Apache License, Version 2.0
# (the "License"); you may not use this file except in compliance with
# the License.  You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Starts up a Pegasus Planner

set -e
set -u

BUILD_TYPE=release
PLANNER_ARGS=""
BINARY_BASE_DIR=${PEGASUS_HOME}/build

# Everything except for -build_type should be passed as a plannerd argument
for ARG in $*
do
  case "$ARG" in
    -build_type=debug)
      BUILD_TYPE=debug
      ;;
    -build_type=release)
      BUILD_TYPE=release
      ;;
    -build_type=*)
      echo "Invalid build type. Valid values are: debug, release"
      exit 1
      ;;
    *)
      PLANNER_ARGS="${PLANNER_ARGS} ${ARG}"
  esac
done

set -u

exec ${BINARY_BASE_DIR}/${BUILD_TYPE}/planner/plannerd ${PLANNER_ARGS}

