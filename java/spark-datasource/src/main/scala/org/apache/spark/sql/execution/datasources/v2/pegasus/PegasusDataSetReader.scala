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
package org.apache.spark.sql.execution.datasources.v2.pegasus

import scala.collection.JavaConverters._

import org.apache.arrow.memory.{BufferAllocator, RootAllocator}
import org.apache.hadoop.conf.Configuration
import org.apache.pegasus.rpc.{FlightDescriptor, FlightInfo, Location, Ticket}
import org.apache.spark.internal.config.ConfigBuilder
import org.apache.spark.sql.util.CaseInsensitiveStringMap

class PegasusDataSetReader(
    hadoopConf: Configuration,
    paths: Seq[String],
    options: CaseInsensitiveStringMap) {

  private[spark] val PLANNER_HOST = ConfigBuilder("planner.host")
    .doc("Hostname of the planner.")
    .stringConf
    .createWithDefault("localhost")

  private[spark] val PLANNER_PORT = ConfigBuilder("planner.port")
    .doc("Port of the planner.")
    .stringConf
    .createWithDefault("30001")

  private[spark] val USERNAME = ConfigBuilder("username")
    .doc("username to access the planner.")
    .stringConf
    .createWithDefault("anonymous")

  private[spark] val PASSWORD = ConfigBuilder("password")
    .doc("password to access the planner.")
    .stringConf
    .createWithDefault("")

  private val rootAllocator = new RootAllocator(Long.MaxValue)
  private val allocator: BufferAllocator = rootAllocator.newChildAllocator(paths.toString(),
    0, rootAllocator.getLimit)

  private val plannerHost = options.get(PLANNER_HOST.key)
  private val plannerPort = options.get(PLANNER_PORT.key)
  private val location = Location.forGrpcInsecure(plannerHost, plannerPort.toInt)

  private val userName = options.get(USERNAME.key)
  private val passWord = options.get(PASSWORD.key)

  private val clientFactory = new PegasusClientFactory(
    allocator, location, userName, passWord)

  def getDataSet(): FlightInfo = {
    try {
      val client = clientFactory.apply
      val descriptor: FlightDescriptor = FlightDescriptor.path(paths.asJava, options)
      client.getInfo(descriptor)
    } catch {
        case e: InterruptedException =>
          throw new RuntimeException(e)
    }
  }

}
