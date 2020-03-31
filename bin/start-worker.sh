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

# Starts up a Pegasus Worker

if [ -z "${PEGASUS_HOME}" ]; then
  export PEGASUS_HOME="$(cd "`dirname "$0"`"/..; pwd)"
fi

# Load the PEGASUS configuration
. "${PEGASUS_HOME}/bin/pegasus-config.sh"
. "${PEGASUS_HOME}/bin/load-pegasus-env.sh"

BUILD_TYPE=release
WORKER_ARGS=""
BINARY_BASE_DIR=${PEGASUS_HOME}/cpp/build

# Everything except for -build_type should be passed as a plannerd argument
for ARG in $*
do
  case "$ARG" in
    -build_type=debug)
      BUILD_TYPE=debug
      ;;
    -build_type=release)
      BUILD_TYPE=release
      ;;
    -build_type=*)
      echo "Invalid build type. Valid values are: debug, release"
      exit 1
      ;;
    *)
      WORKER_ARGS="${WORKER_ARGS} ${ARG}"
  esac
done

# Find the port number for the worker
if [ "$PEGASUS_WORKER_PORT" = "" ]; then
  PEGASUS_WORKER_PORT=30002
fi

if [ "$PEGASUS_WORKER_HOST" = "" ]; then
  case `uname` in
      (SunOS)
	  PEGASUS_WORKER_HOST="`/usr/sbin/check-hostname | awk '{print $NF}'`"
	  ;;
      (*)
	  PEGASUS_WORKER_HOST="`hostname -f`"
	  ;;
  esac
fi

if [ "$STORE_TYPES" = "" ]; then
  STORE_TYPES="MEMORY"
fi

if [ "$STORE_DRAM_CAPACITY_GB" = "" ]; then
  STORE_DRAM_CAPACITY_GB=10
fi

if [ "$STORE_DCPMM_INITIAL_CAPACITY_GB" = "" ]; then
  STORE_DCPMM_INITIAL_CAPACITY_GB=100
fi

if [ "$STORE_DCPMM_RESERVED_CAPACITY_GB" = "" ]; then
  STORE_DCPMM_RESERVED_CAPACITY_GB=40
fi

if [ "$STORE_CACHE_RATIO" = "" ]; then
  STORE_CACHE_RATIO=0.8
fi

if [ "$LRU_CACHE_CAPACITY_MB" = "" ]; then
  LRU_CACHE_CAPACITY_MB=512
fi

if [ "$LRU_CACHE_TYPE" = "" ]; then
  LRU_CACHE_TYPE="DRAM"
fi

if [ "$PEGASUS_PID_DIR" = "" ]; then
  PEGASUS_PID_DIR=/tmp
fi

pid="$PEGASUS_PID_DIR/pegasus-worker.pid"

function start_worker() {
  mkdir -p "$PEGASUS_PID_DIR"

  if [ -f "$pid" ]; then
    TARGET_ID="$(cat "$pid")"
    if [[ $(ps -p "$TARGET_ID" -o comm=) =~ "workerd" ]]; then
      echo "pegasus worker running as process $TARGET_ID.  Stop it first."
      exit 1
    fi
  fi

  WORKER_ARGS="$WORKER_ARGS --hostname $PEGASUS_WORKER_HOST"
  WORKER_ARGS="$WORKER_ARGS --worker_port $PEGASUS_WORKER_PORT"
  WORKER_ARGS="$WORKER_ARGS --store_types $STORE_TYPES"
  if [ "$STORE_DRAM_ENABLED" = "true" ]; then
    WORKER_ARGS="$WORKER_ARGS --store_dram_enabled"
  else
    WORKER_ARGS="$WORKER_ARGS --store_dram_enabled=false"
  fi
  WORKER_ARGS="$WORKER_ARGS --store_dram_capacity_gb $STORE_DRAM_CAPACITY_GB"
  if [ "$STORE_DCPMM_ENABLED" = "true" ]; then
    WORKER_ARGS="$WORKER_ARGS --store_dcpmm_enabled"
  fi
  if [ "$STORAGE_DCPMM_PATH" != "" ]; then
    WORKER_ARGS="$WORKER_ARGS --storage_dcpmm_path $STORAGE_DCPMM_PATH"
  fi
  WORKER_ARGS="$WORKER_ARGS --store_dcpmm_initial_capacity_gb $STORE_DCPMM_INITIAL_CAPACITY_GB"
  WORKER_ARGS="$WORKER_ARGS --store_dcpmm_reserved_capacity_gb $STORE_DCPMM_RESERVED_CAPACITY_GB"
  WORKER_ARGS="$WORKER_ARGS --store_cache_ratio $STORE_CACHE_RATIO"
  if [ "$FORCE_LRU_CACHE_CAPACITY" = "true" ]; then
    WORKER_ARGS="$WORKER_ARGS --force_lru_cache_capacity"
  fi
  WORKER_ARGS="$WORKER_ARGS --lru_cache_capacity_mb $LRU_CACHE_CAPACITY_MB"
  WORKER_ARGS="$WORKER_ARGS --lru_cache_type $LRU_CACHE_TYPE"
  if [ "$CACHE_FORCE_SINGLE_SHARD" = "true" ]; then
    WORKER_ARGS="$WORKER_ARGS --cache_force_single_shard"
  fi

  exec ${BINARY_BASE_DIR}/${BUILD_TYPE}/worker/workerd ${WORKER_ARGS} &
  newpid="$!"
  echo "$newpid" > "$pid"

  # Poll for up to 10 seconds for the worker to start
  for i in {1..10}
  do
    sleep 1
    if [[ $(ps -p "$newpid" -o comm=) =~ "workerd" ]]; then
      break
    fi
  done
}

# Start Worker
start_worker
