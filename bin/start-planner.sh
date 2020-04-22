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

# Starts up a Pegasus Planner

if [ -z "${PEGASUS_HOME}" ]; then
  export PEGASUS_HOME="$(cd "`dirname "$0"`"/..; pwd)"
fi

# Load the PEGASUS configuration
. "${PEGASUS_HOME}/bin/pegasus-config.sh"
. "${PEGASUS_HOME}/bin/load-pegasus-env.sh"

BUILD_TYPE=release
PLANNER_ARGS=""
BINARY_BASE_DIR=${PEGASUS_HOME}/cpp/build


ARGS=`getopt -o awh:pp:wp:ph:bt: --long along,worker_hostname:,planner_port:,worker_port:,planner_hostname:,build_type: -- $@`
if [ $? != 0 ]; then
    echo "Planner terminating..."
    exit 1
fi


#Normalize command line parameters
eval set -- "${ARGS}"

while true
do
    case "$1" in
        -a|--along) 
            echo "Option a";
            shift
            ;;
        -wh|--worker_hostname)
            PEGASUS_WORKER_HOST=$2
            shift 2
            ;;
        -pp|--planner_port)
            case "$2" in
                "")
                    echo "Option pp, no argument";
                    shift 2  
                    ;;
                *)
                    PEGASUS_PLANNER_PORT=$2
                    shift 2;
                    ;;
            esac
            ;;

        -wp|--worker_port)
            case "$2" in
                "")
                    echo "Option wp, no argument";
                    shift 2  
                    ;;
                *)
                    PEGASUS_WORKER_PORT=$2
                    shift 2;
                    ;;
            esac
            ;;
        -ph|--planner_hostname)
            case "$2" in
                "")
                    echo "Option ph, no argument";
                    shift 2  
                    ;;
                *)
                    PEGASUS_PLANNER_HOST=$2
                    shift 2;
                    ;;
            esac
            ;;
        -bt|--build_type)
            case "$2" in
                "")
                    echo "Option bt, no argument";
                    shift 2  
                    ;;
                *)
                    BUILD_TYPE=$2
                    shift 2;
                    ;;
            esac
            ;;
        --)
            shift
            break
            ;;
        *)
            echo "Internal error!"
            exit 1
            ;;
    esac
done

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

if [ "$PEGASUS_PID_DIR" = "" ]; then
  PEGASUS_PID_DIR=/tmp
fi

pid="$PEGASUS_PID_DIR/pegasus-planner.pid"

function start_planner() {
  mkdir -p "$PEGASUS_PID_DIR"

  if [ -f "$pid" ]; then
    TARGET_ID="$(cat "$pid")"
    if [[ $(ps -p "$TARGET_ID" -o comm=) =~ "plannerd" ]]; then
      echo "pegasus planner running as process $TARGET_ID.  Stop it first."
      exit 1
    fi
  fi

  PLANNER_ARGS="$PLANNER_ARGS --hostname $PEGASUS_PLANNER_HOST"
  PLANNER_ARGS="$PLANNER_ARGS --planner_port $PEGASUS_PLANNER_PORT"
  if [ "$CHECK_DATASET_APPEND_ENABLED" = "true" ]; then
    PLANNER_ARGS="$PLANNER_ARGS --check_dataset_append_enabled"
  else
    PLANNER_ARGS="$PLANNER_ARGS --check_dataset_append_enabled=false"
  fi
  exec ${BINARY_BASE_DIR}/${BUILD_TYPE}/planner/plannerd ${PLANNER_ARGS} &
  newpid="$!"
  echo "$newpid" > "$pid"
  # Poll for up to 10 seconds for the worker to start
  for i in {1..10}
  do
    sleep 1
    if [[ $(ps -p "$newpid" -o comm=) =~ "plannerd" ]]; then
      break
    fi
  done
}

# Start Planner
start_planner



