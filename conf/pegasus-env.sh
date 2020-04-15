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

# Options for the daemons used in the standalone deploy mode
# - PEGASUS_PLANNER_HOST   to bind the planner to a different IP address or hostname
# - PEGASUS_PLANNER_PORT   to use non-default ports for the planner
# - PEGASUS_WORKER_PORT    to use non-default ports for the worker
export PEGASUS_PLANNER_PORT=30001
export PEGASUS_WORKER_PORT=30002

# Generic options for the daemons used in the standalone deploy mode
# - PEGASUS_CONF_DIR      Alternate conf dir. (Default: ${PEGASUS_HOME}/conf)
# - PEGASUS_LOG_DIR       Where log files are stored.  (Default: ${PEGASUS_HOME}/logs)

# STORE options
# - STORE_TYPES                Store types. (Default: MEMORY)
# - STORE_DRAM_ENABLED         If true, enable DRAM store. (Default: true)
# - STORE_DRAM_CAPACITY_GB     The DRAM store capacity in GB. (Default: 10)
# - STORE_DCPMM_ENABLED        If true, enable DCPMM store. (Default: false)
# - STORAGE_DCPMM_PATH         The DCPMM device path.
# - STORE_DCPMM_INITIAL_CAPACITY_GB    The DCPMM store initial capacity in GB.(Default: 100)
# - STORE_DCPMM_RESERVED_CAPACITY_GB    The DCPMM store reserved capacity in GB.(Default: 40)
# - STORE_CACHE_RATIO    The cache available ratio.(Default: 0.8)
# - FORCE_LRU_CACHE_CAPACITY   Force pegasus to accept the lru cache size, even if it is unsafe.(Default: true))
# - LRU_CACHE_CAPACITY_MB      lru cache capacity in MB.(Default: 512)
# - LRU_CACHE_TYPE             Which type of lru cache to use for caching data.(Default: DRAM)
# - CACHE_FORCE_SINGLE_SHARD   Override all cache implementations to use just one shard.(Default: ture)

# - CHECK_DATASET_APPEND_ENABLED    If true, check the dataset append. (Default: true)
export CHECK_DATASET_APPEND_ENABLED=false
