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
import org.apache.pegasus.rpc.FlightClient
import org.apache.pegasus.rpc.Location

class PegasusClientFactory(location: Location,
                           username: String,
                           password: String) extends AutoCloseable {

  private val allocator = new RootAllocator(Long.MaxValue)
  //TODO make useFileBatch configurable
  private var useFileBatch = false

  def apply: FlightClient = {
    var client: FlightClient = null
    if(useFileBatch) {
      client = FlightClient.builder(allocator, location).useFileBatch().build
    } else {
      client = FlightClient.builder(allocator, location).build
    }
//    client.authenticateBasic(username, password)
    client
  }

  def getUsername: String = username

  def getPassword: String = password

  @throws[Exception]
  override def close(): Unit = {
    allocator.close()
  }
}
