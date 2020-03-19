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

# Run a shell command on all worker hosts.
#
# Environment Variables
#
#   PEGASUS_WORKERS    File naming remote hosts.
#     Default is ${PEGASUS_CONF_DIR}/workers.
#   PEGASUS_CONF_DIR  Alternate conf dir. Default is ${PEGASUS_HOME}/conf.
#   PEGASUS_WORKER_SLEEP Seconds to sleep between spawning remote commands.
#   PEGASUS_SSH_OPTS Options passed to ssh when running remote commands.
##

usage="Usage: workers.sh [--config <conf-dir>] command..."

# if no args specified, show usage
if [ $# -le 0 ]; then
  echo $usage
  exit 1
fi

if [ -z "${PEGASUS_HOME}" ]; then
  export PEGASUS_HOME="$(cd "`dirname "$0"`"/..; pwd)"
fi

. "${PEGASUS_HOME}/bin/pegasus-config.sh"

# If the workers file is specified in the command line,
# then it takes precedence over the definition in
# pegasus-env.sh. Save it here.
if [ -f "$PEGASUS_WORKERS" ]; then
  HOSTLIST=`cat "$PEGASUS_WORKERS"`
fi

# Check if --config is passed as an argument. It is an optional parameter.
# Exit if the argument is not a directory.
if [ "$1" == "--config" ]
then
  shift
  conf_dir="$1"
  if [ ! -d "$conf_dir" ]
  then
    echo "ERROR : $conf_dir is not a directory"
    echo $usage
    exit 1
  else
    export PEGASUS_CONF_DIR="$conf_dir"
  fi
  shift
fi

if [ "$HOSTLIST" = "" ]; then
  if [ "$PEGASUS_WORKERS" = "" ]; then
    if [ -f "${PEGASUS_CONF_DIR}/workers" ]; then
      HOSTLIST=`cat "${PEGASUS_CONF_DIR}/workers"`
    else
      HOSTLIST=localhost
    fi
  else
    HOSTLIST=`cat "${PEGASUS_WORKERS}"`
  fi
fi


# By default disable strict host key checking
if [ "$PEGASUS_SSH_OPTS" = "" ]; then
  PEGASUS_SSH_OPTS="-o StrictHostKeyChecking=no"
fi

for worker in `echo "$HOSTLIST"|sed  "s/#.*$//;/^$/d"`; do
  if [ -n "${PEGASUS_SSH_FOREGROUND}" ]; then
    ssh $PEGASUS_SSH_OPTS "$worker" $"${@// /\\ }" \
      2>&1 | sed "s/^/$worker: /"
  else
    ssh $PEGASUS_SSH_OPTS "$worker" $"${@// /\\ }" \
      2>&1 | sed "s/^/$worker: /" &
  fi
  if [ "$PEGASUS_WORKER_SLEEP" != "" ]; then
    sleep $PEGASUS_WORKER_SLEEP
  fi
done

wait
