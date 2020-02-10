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

import org.apache.arrow.memory.{BufferAllocator, RootAllocator}
import org.apache.pegasus.rpc.{Action, FlightClient, FlightDescriptor, FlightEndpoint, FlightInfo, Location, Result, SchemaResult}
import org.apache.spark.sql.util.CaseInsensitiveStringMap

class PegasusDataSetReader(options: CaseInsensitiveStringMap, paths: Seq[String]) {

  private val rootAllocator = new RootAllocator(Long.MaxValue)
  private val allocator: BufferAllocator = rootAllocator.newChildAllocator(options.toString, 0, rootAllocator.getLimit)

  private val location = Location.forGrpcInsecure(
    options.getOrDefault("host", "localhost"),
    options.getInt("port", 30001))
  private val clientFactory = new PegasusClientFactory(
    allocator, location, options.getOrDefault("username", "anonymous"),
    options.getOrDefault("password", null))

  def getDataSet(): FlightInfo = {
    try {
      val client = clientFactory.apply
      client.getInfo(FlightDescriptor.path(paths.mkString(" ")))
    } catch {
        case e: InterruptedException =>
          throw new RuntimeException(e)
    }
  }

  def pegasusPartitionReader(pegasusPartition: PegasusPartition): PegasusPartitionReader = {

    val endpoint = pegasusPartition.endpoint
    val locations = endpoint.getLocations
    var location: Location = null
    if (locations.isEmpty) {
      location = Location.forGrpcInsecure(location.getUri.getHost, location.getUri.getPort)
    } else {
      location = endpoint.getLocations.get(0)
    }
    new PegasusPartitionReader(endpoint.getTicket.getBytes, location.getUri.getHost, location.getUri.getPort,
      clientFactory.getUsername, clientFactory.getPassword)
  }
}
