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

# build arrow and install
sh $PEGASUS_HOME/dev/build_arrow.sh

# build spark and install
sh $PEGASUS_HOME/dev/build_spark.sh

# build pegasus server offline
sh $PEGASUS_HOME/dev/build_pegasus.sh

# build pegasus java for Spark
sh $PEGASUS_HOME/dev/build_pegasus_java.sh
