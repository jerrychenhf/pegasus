#!/bin/bash
#
# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.
# The ASF licenses this file to You under the Apache License, Version 2.0
# (the "License"); you may not use this file except in compliance with
# the License.  You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
WORK_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
CURRENT_DIR=$(pwd)

source $WORK_DIR/set_project_home.sh

#set -eu

# check cmake version
CMAKE_TARGET_VERSION=3.7.1
CMAKE_MIN_VERSION=3.7
TARGET_CMAKE_SOURCE_URL=https://cmake.org/files/v3.7/cmake-3.7.1.tar.gz

function version_lt() { test "$(echo "$@" | tr " " "\n" | sort -rV | head -n 1)" != "$1"; }
function version_ge() { test "$(echo "$@" | tr " " "\n" | sort -rV | head -n 1)" == "$1"; }

function download_and_install_cmake() {
      mkdir -p $THIRD_PARTY_DIR
      cd $THIRD_PARTY_DIR
      mkdir -p cmake
      cd cmake
      echo "Will use $THIRD_PARTY_DIR/cmake/cmake-$CMAKE_TARGET_VERSION.tar.gz"
      if [ ! -f "$THIRD_PARTY_DIR/cmake/cmake-$CMAKE_TARGET_VERSION.tar.gz" ]; then
        wget $TARGET_CMAKE_SOURCE_URL
      fi
      tar xvf cmake-$CMAKE_TARGET_VERSION.tar.gz
      cd cmake-$CMAKE_TARGET_VERSION/
      ./bootstrap
      gmake
      gmake install
}

function prepare_cmake() {
  CURRENT_CMAKE_VERSION_STR="$(cmake --version)"

  if [[ "$CURRENT_CMAKE_VERSION_STR" == "cmake version"* ]]; then
    echo "cmake is installed"
    array=(${CURRENT_CMAKE_VERSION_STR//,/ })
    CURRENT_CMAKE_VERSION=${array[2]}
    if version_lt $CURRENT_CMAKE_VERSION $CMAKE_MIN_VERSION; then
      echo "$CURRENT_CMAKE_VERSION is less than $CMAKE_MIN_VERSION,install cmake $CMAKE_TARGET_VERSION"
      download_and_install_cmake
      yum remove cmake -y
      ln -s /usr/local/bin/cmake /usr/bin/
      cd $CURRENT_DIR
    fi
  else
    echo "cmake is not installed"
    download_and_install_cmake
    cd $CURRENT_DIR
  fi
}

prepare_cmake
