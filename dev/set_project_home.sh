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

if [ -z "${PEGASUS_HOME}" ]; then
  export PEGASUS_HOME="$( cd "$( dirname "${BASH_SOURCE[0]}" )"/.. >/dev/null 2>&1 && pwd )"
  echo "Setup PEGASUS_HOME to $PEGASUS_HOME"
fi

if [ -z "${THIRD_PARTY_DIR}" ]; then
  export THIRD_PARTY_DIR=$PEGASUS_HOME/dev/thirdparty
  echo "Set THIRD_PARTY_DIR to $THIRD_PARTY_DIR"
fi