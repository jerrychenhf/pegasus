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

import org.apache.pegasus.rpc.{Location, Ticket}
import org.apache.spark.internal.Logging
import org.apache.spark.sql.catalyst.InternalRow
import org.apache.spark.sql.connector.read.{InputPartition, PartitionReader, PartitionReaderFactory}
import org.apache.spark.sql.internal.SQLConf
import org.apache.spark.sql.types.{AtomicType, StructType}
import org.apache.spark.sql.vectorized.ColumnarBatch

/**
  * A factory used to create Pegasus Partition readers.
  *
  * * @param paths locations.
  * * @param sqlConf SQL configuration.
  * * @param readDataSchema Required data schema in the batch scan.
  *
  */
case class PegasusPartitionReaderFactory(
    paths: Seq[String],
    sqlConf: SQLConf,
    readDataSchema: StructType) extends PartitionReaderFactory with Logging {

  override def supportColumnarReads(partition: InputPartition): Boolean = {
    sqlConf.wholeStageEnabled &&
      readDataSchema.length <= sqlConf.wholeStageMaxNumFields &&
      readDataSchema.forall(_.dataType.isInstanceOf[AtomicType])
  }

  @throws(classOf[NoSuchElementException])
  override def createColumnarReader(partition: InputPartition): PartitionReader[ColumnarBatch] = {

    assert(partition.isInstanceOf[PegasusPartition])
    val pegasusPartition = partition.asInstanceOf[PegasusPartition]
    val endpoint = pegasusPartition.endpoint
    val locations = endpoint.getLocations
    var location: Location = null
    if (!locations.isEmpty) {
      location = endpoint.getLocations.get(0)
      if (location.getUri != null) {
        if (location.getUri.getHost == null) {
          throw new NoSuchElementException("No host name found in location.")
        }
        if (location.getUri.getPort == null) {
          throw new NoSuchElementException("No host port found in location.")
        }
      } else {
        throw new NoSuchElementException("No Location found in FlightEndpoint.")
      }
    } else {
      throw new NoSuchElementException("No URI found in Location.")
    }

    val ticket: Ticket = endpoint.getTicket

    val pegasusReader = new PegasusPartitionReader(ticket, location, null, null)

    new PartitionReader[ColumnarBatch] {
      override def next(): Boolean = {
        pegasusReader.next
      }

      override def get(): ColumnarBatch = {
        pegasusReader.get
      }

      override def close(): Unit = pegasusReader.close
    }
  }

  override def createReader(partition: InputPartition): PartitionReader[InternalRow] = {
    throw new UnsupportedOperationException
  }
}
