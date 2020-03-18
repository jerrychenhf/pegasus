/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package org.apache.spark.sql.internal

import org.apache.spark.internal.config.ConfigBuilder
import org.apache.spark.network.util.ByteUnit

////////////////////////////////////////////////////////////////////////////////////////////////////
// This file defines the configuration options for PEGASUS.
////////////////////////////////////////////////////////////////////////////////////////////////////


object PegasusConf {

  val PLANNER_HOST =
    SQLConf.buildConf("spark.planner.host")
      .internal()
      .doc("Hostname of the Pegasus Planner.")
      .stringConf
      .createWithDefault("localhost")

  val PLANNER_PORT =
    SQLConf.buildConf("spark.planner.port")
    .internal()
    .doc("Port of the Pegasus Planner.")
    .stringConf
    .createWithDefault("30001")

  val USERNAME =
    SQLConf.buildConf("spark.planner.username")
    .internal()
    .doc("username to access the planner.")
    .stringConf
    .createWithDefault("anonymous")

  val PASSWORD =
    SQLConf.buildConf("spark.planner.password")
    .internal()
    .doc("password to access the planner.")
    .stringConf
    .createWithDefault("")
}
