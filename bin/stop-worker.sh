#!/usr/bin/env bash

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

# Stops the master on the machine this script is executed on.

if [ -z "${PEGASUS_HOME}" ]; then
  export PEGASUS_HOME="$(cd "`dirname "$0"`"/..; pwd)"
fi

. "${PEGASUS_HOME}/bin/pegasus-config.sh"


if [ "$PEGASUS_PID_DIR" = "" ]; then
  PEGASUS_PID_DIR=/tmp
fi

pid="$PEGASUS_PID_DIR/pegasus-worker.pid"

if [ -f $pid ]; then
  TARGET_ID="$(cat "$pid")"
  if [[ $(ps -p "$TARGET_ID" -o comm=) =~ "workerd" ]]; then
    echo "stopping worker"
    kill "$TARGET_ID" && rm -f "$pid"
  else
    echo "no worker to stop"
  fi
else
  echo "no worker to stop"
fi
