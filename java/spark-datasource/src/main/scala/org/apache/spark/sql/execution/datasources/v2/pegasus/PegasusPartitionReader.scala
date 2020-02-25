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

import org.apache.pegasus.rpc.FlightClient
import org.apache.pegasus.rpc.Location
import org.apache.pegasus.rpc.Ticket
import org.apache.arrow.memory.RootAllocator
import org.apache.spark.sql.vectorized.{ArrowColumnVector, ColumnVector, ColumnarBatch}

class PegasusPartitionReader(ticket: Ticket,
                             defaultHost: String,
                             defaultPort: Int,
                             username: String,
                             password: String) {

  private val allocator = new RootAllocator

  private val client = FlightClient.builder(allocator, Location.forGrpcInsecure(defaultHost, defaultPort)).build();
  private val stream = client.getStream(ticket)

  def next: Boolean = {
    return stream.next()
  }

  def get: ColumnarBatch = {
    val columns = stream.getRoot().getFieldVectors().asScala.map { vector =>
      new ArrowColumnVector(vector).asInstanceOf[ColumnVector]
    }.toArray

    val batch = new ColumnarBatch(columns)
    batch.setNumRows(stream.getRoot().getRowCount())
    batch
  }

  def close = {

  }
}