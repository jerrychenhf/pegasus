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

# check java
JAVA_PACKAGE_FILE=jdk-8u261-linux-x64.tar.gz
JAVA_DOWNLOAD_URL=https://enos.itcollege.ee/~jpoial/allalaadimised/jdk8/$JAVA_PACKAGE_FILE

CURRENT_JAVA_VERSION_STR="$(java -version)"
if [[ "$CURRENT_JAVA_VERSION_STR" == "1java version "* ]]; then
  echo "Java is installed in the system."
else
  echo "Java is not installed in the system. We will download and install one."
  
   if [ ! -d "$THIRD_PARTY_DIR/java/apache-maven-$MAVEN_VERSION" ]; then
    mkdir -p $THIRD_PARTY_DIR
    cd $THIRD_PARTY_DIR
    mkdir -p java
    cd java
    echo "Will use $THIRD_PARTY_DIR/java/$JAVA_PACKAGE_FILE"
    if [ ! -f "$JAVA_PACKAGE_FILE" ]; then
      wget $JAVA_DOWNLOAD_URL
    fi
    
    gunzip $JAVA_PACKAGE_FILE
    tar -xf jdk-8u261-linux-x64.tar -C $THIRD_PARTY_DIR/java
  fi
  
  export JAVA_HOME=$THIRD_PARTY_DIR/java/jdk1.8.0_261
  PATH=$PATH:$JAVA_HOME/bin
  export PATH
  
  cd $CURRENT_DIR
fi
