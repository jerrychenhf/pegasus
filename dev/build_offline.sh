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

ACTION=build

while [[ $# -gt 0 ]]
do
key="$1"
case $key in
    --rebuild)
    shift 1 # past argument
    ACTION=rebuild
    ;;
    *)    # completed this shell arugemnts procesing
    break
    ;;
esac
done

$PEGASUS_HOME/dev/install_dependencies_offline.sh
source $PEGASUS_HOME/dev/set_dependancy_urls.sh

case "$ACTION" in
  rebuild)
    rm -rf $PEGASUS_HOME/cpp/build
    ;;
  *)
    ;;
esac

if [ ! -d "$PEGASUS_HOME/cpp/build" ]; then
  mkdir -p $PEGASUS_HOME/cpp/build
fi

cd  $PEGASUS_HOME/cpp/build
cmake -DPEGASUS_USE_GLOG=ON -DPEGASUS_BUILD_TESTS=ON -DPEGASUS_DEPENDENCY_SOURCE=AUTO ..
make
