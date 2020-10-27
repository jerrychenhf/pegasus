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
CURRENT_DIR=$(pwd)

source $WORK_DIR/set_project_home.sh

#set -eu

# complile spark from source
function build_spark_from_source(){
      if [ ! -d "./spark" ]; then
        git clone https://github.com/Intel-bigdata/spark.git -b branch-3.0-pegasus
      fi
      cd spark
      mvn -DskipTests clean install
      cd $CURRENT_DIR
}

REBUILD=false

while [[ $# -gt 0 ]]
do
key="$1"
case $key in
    --rebuild)
    shift 1 # past argument
    REBUILD=true
    ;;
    *)    # completed this shell arugemnts procesing
    break
    ;;
esac
done

if [ "$REBUILD" = true ] ; then
    build_spark_from_source
else
     if [ ! -d "./spark" ]; then
        build_spark_from_source
     fi
fi
