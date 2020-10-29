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

# check cmake version
MAVEN_VERSION=3.6.3
MAVEN_DOWNLOAD_URL=https://mirrors.sonic.net/apache/maven/maven-3/3.6.3/binaries/apache-maven-3.6.3-bin.tar.gz

CURRENT_MAVEN_VERSION_STR="$(mvn --version)"
if [[ "$CURRENT_MAVEN_VERSION_STR" == "Apache Maven "* ]]; then
  echo "Maven is installed in the system"
else
  echo "Maven is not installed in the system. Will download one."
  
   if [ ! -d "$THIRD_PARTY_DIR/maven/apache-maven-$MAVEN_VERSION" ]; then
    mkdir -p $THIRD_PARTY_DIR
    cd $THIRD_PARTY_DIR
    mkdir -p maven
    cd maven
    echo "Will use $THIRD_PARTY_DIR/maven/apache-maven-$MAVEN_VERSION-bin.tar.gz"
    if [ ! -f "apache-maven-$MAVEN_VERSION-bin.tar.gz" ]; then
      wget $MAVEN_DOWNLOAD_URL
    fi
  
    tar xvf apache-maven-$MAVEN_VERSION-bin.tar.gz
  fi
  
  export MAVEN_HOME=$THIRD_PARTY_DIR/maven/apache-maven-$MAVEN_VERSION
  PATH=$PATH:$MAVEN_HOME/bin
  export PATH
  
  cd $CURRENT_DIR
fi
