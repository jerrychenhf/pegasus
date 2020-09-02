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

import scala.collection.JavaConverters._
import org.apache.pegasus.rpc.{FlightClient, FlightFileBatchStream, LocalPartitionReader, Ticket}
import org.apache.spark.internal.Logging
import org.apache.spark.sql.vectorized.{ArrowColumnVector, ColumnVector}

class GrpcDataReader(client: FlightClient,
                     ticket: Ticket) extends DataReader with Logging {

  val stream = client.getStream(ticket)

  override def next: Boolean = {
    stream.next
  }

  override def get: util.List[ByteBuffer] = {
    stream.asInstanceOf[FlightFileBatchStream].getFileBatchRoot().getbyteBuffers()
  }

  def getColumns: Array[ColumnVector] = {
    stream.getRoot().getFieldVectors().asScala.map { vector =>
      new ArrowColumnVector(vector).asInstanceOf[ColumnVector]
    }.toArray
  }

  override def getBatchCount:  Int = {
    stream.asInstanceOf[FlightFileBatchStream].getFileBatchRoot().getRowCount()
  }

  override def close = {
    stream.close()
  }

}
