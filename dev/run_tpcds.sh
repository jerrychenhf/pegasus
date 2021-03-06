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

export BEAVER_HOME=/opt/Beaver

# first stop
sh $PEGASUS_HOME/bin/stop-all.sh
sleep 10

# then build
sh $PEGASUS_HOME/dev/build_all.sh

# start the pegasus services
sh $PEGASUS_HOME/bin/start-all.sh

# update the jars to Beaver
cp -r  $PEGASUS_HOME/java/spark-datasource/target/pegasus-spark-datasource-1.0.0-SNAPSHOT-with-spark-3.0.0.jar    $BEAVER_HOME/OAP/oap_jar/

# run tpc-ds
cd $BEAVER_HOME/spark-sql-perf/tpcds_script/3.0.0/
sh run_tpcds_sparksql.sh  4