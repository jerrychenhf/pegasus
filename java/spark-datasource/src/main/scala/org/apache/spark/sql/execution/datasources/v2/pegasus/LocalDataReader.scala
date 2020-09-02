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

import java.nio.ByteBuffer
import java.util
import java.util.List

import org.apache.pegasus.rpc.{FlightClient, LocalPartitionReader, Ticket}
import org.apache.spark.internal.Logging

class LocalDataReader(client: FlightClient,
                      ticket: Ticket) extends DataReader with Logging {

  val localPartitionInfo = client.getLocalData(ticket)
  val localPartitionReader = new LocalPartitionReader(localPartitionInfo)
  localPartitionReader.open()

  override def next: Boolean = {
    localPartitionReader.next
  }

  override def get: util.List[ByteBuffer] = {
    localPartitionReader.get
  }

  override def getBatchCount:  Int = {
    // TODO
    localPartitionReader.getRowCount
  }

  override def close = {
    //TODO
    client.releaseLocalData(ticket)
    localPartitionReader.close()
  }

}
