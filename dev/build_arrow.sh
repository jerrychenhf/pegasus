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

$PEGASUS_HOME/dev/install_dependencies_offline.sh
source $PEGASUS_HOME/dev/set_dependancy_urls.sh

source $PEGASUS_HOME/dev/check_maven.sh

MVN_CMD="mvn"
if [ ! -z "${MAVEN_PROXY_OPTS}" ]; then
  MVN_CMD="mvn ${MAVEN_PROXY_OPTS}"
fi

REBUILD=false

# install ARROW
function build_arrow_from_source(){
      if [ ! -d "./arrow" ]; then
        git clone https://github.com/Intel-bigdata/arrow.git -b branch-1.0-pegasus
      fi
      cd arrow/cpp

      if [ "$REBUILD" = true ] ; then
        rm -rf ./build-arrow
      fi
      
       if [ ! -d "./build-arrow" ]; then
          mkdir -p build-arrow
       fi
      cd build-arrow
      yum install openssl-devel flex bison -y
      cmake -DARROW_FLIGHT=ON -DARROW_PARQUET=ON -DARROW_HDFS=ON -DARROW_WITH_SNAPPY=ON -DARROW_BUILD_TESTS=ON -DARROW_DEPENDENCY_SOURCE=AUTO ..
      make
      make install
      cp -r $(pwd)/src/arrow/filesystem /usr/local/include/arrow/
      cd ../../java
      
      if [ "$REBUILD" = true ] ; then
        $MVN_CMD -DskipTests clean install
      else
        $MVN_CMD -DskipTests install
      fi

      cd $CURRENT_DIR
      echo "export ARROW_HOME=/usr/local" >> ~/.bashrc
      source ~/.bashrc
}

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

build_arrow_from_source
 
