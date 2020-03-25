#!/bin/sh

# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.
# The ASF licenses this file to You under the Apache License, Version 2.0
# (the "License"); you may not use this file except in compliance with
# the License.  You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Starts a worker on each node specified in conf/workers

if [ -z "${PEGASUS_HOME}" ]; then
  export PEGASUS_HOME="$(cd "`dirname "$0"`"/..; pwd)"
fi

. "${PEGASUS_HOME}/bin/pegasus-config.sh"
. "${PEGASUS_HOME}/bin/load-pegasus-env.sh"

# Find the port number for the planner
if [ "$PEGASUS_PLANNER_PORT" = "" ]; then
  PEGASUS_PLANNER_PORT=30001
fi

if [ "$PEGASUS_PLANNER_HOST" = "" ]; then
  case `uname` in
      (SunOS)
	  PEGASUS_PLANNER_HOST="`/usr/sbin/check-hostname | awk '{print $NF}'`"
	  ;;
      (*)
	  PEGASUS_PLANNER_HOST="`hostname -f`"
	  ;;
  esac
fi

# Launch the workers
"${PEGASUS_HOME}/bin/workers.sh" cd "${PEGASUS_HOME}" \;"${PEGASUS_HOME}/bin/start-worker.sh"  "--planner_hostname $PEGASUS_PLANNER_HOST --planner_port $PEGASUS_PLANNER_PORT"


