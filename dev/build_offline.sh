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

WORK_DIR="$(dirname "$0")"

export PEGASUS_HOME=$WORK_DIR/../

cd $PEGASUS_HOME/cpp
source ./pegasus_env.conf 
source ./arrow_env.conf

if [ ! -d "$PEGASUS_HOME/cpp/build" ]; then
  mkdir build
fi

cd  $PEGASUS_HOME/cpp/build
cmake -DPEGASUS_USE_GLOG=ON ..
make
