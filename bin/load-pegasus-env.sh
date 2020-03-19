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

# This script loads pegasus-env.sh if it exists, and ensures it is only loaded once.
# pegasus-env.sh is loaded from PEGASUS_CONF_DIR if set, or within the current directory's
# conf/ subdirectory.

# Figure out where Pegasus is installed
if [ -z "${PEGASUS_HOME}" ]; then
  source "$(dirname "$0")"/find-pegasus-home
fi

PEGASUS_ENV_SH="pegasus-env.sh"
if [ -z "$PEGASUS_ENV_LOADED" ]; then
  export PEGASUS_ENV_LOADED=1

  export PEGASUS_CONF_DIR="${PEGASUS_CONF_DIR:-"${PEGASUS_HOME}"/conf}"

  PEGASUS_ENV_SH="${PEGASUS_CONF_DIR}/${PEGASUS_ENV_SH}"
  if [[ -f "${PEGASUS_ENV_SH}" ]]; then
    # Promote all variable declarations to environment (exported) variables
    set -a
    . ${PEGASUS_ENV_SH}
    set +a
  fi
fi

