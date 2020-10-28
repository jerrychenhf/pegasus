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
WORK_DIR="$(cd "`dirname "$0"`"; pwd)"

source $WORK_DIR/set_project_home.sh

#set -eu

$PEGASUS_HOME/dev/check_cmake.sh

ARROW_THIRD_PARTY_DIR=$THIRD_PARTY_DIR/arrow-thirdparty
if [ ! -d "$ARROW_THIRD_PARTY_DIR" ]; then
  mkdir -p $ARROW_THIRD_PARTY_DIR
fi

echo "Downloading Arrow dependencies..."
sh $PEGASUS_HOME/cpp/thirdparty/download_arrow_dependencies.sh $ARROW_THIRD_PARTY_DIR > $THIRD_PARTY_DIR/arrow_env.conf
echo "Finished downloading Arrow dependencies."

PEGASUS_THIRD_PARTY_DIR=$THIRD_PARTY_DIR/pegasus-thirdparty
if [ ! -d "$PEGASUS_THIRD_PARTY_DIR" ]; then
  mkdir -p $PEGASUS_THIRD_PARTY_DIR
fi

echo "Downloading Pegasus dependencies..."
sh $PEGASUS_HOME/cpp/thirdparty/download_dependencies.sh $PEGASUS_THIRD_PARTY_DIR > $THIRD_PARTY_DIR/pegasus_env.conf
echo "Finished downloading Pegasus dependencies."
