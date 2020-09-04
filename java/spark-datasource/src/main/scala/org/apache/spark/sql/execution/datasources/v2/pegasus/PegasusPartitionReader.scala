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

import java.io.IOException
import java.net.NetworkInterface
import java.util

import scala.collection.JavaConverters._
import org.apache.pegasus.rpc.{FlightClient, FlightFileBatchStream, Location, Ticket}
import org.apache.arrow.memory.RootAllocator
import org.apache.arrow.util.AutoCloseables
import org.apache.hadoop.conf.Configuration
import org.apache.hadoop.fs.Path
import org.apache.parquet.format.converter.ParquetMetadataConverter.NO_FILTER
import org.apache.parquet.hadoop.{ParquetFileReader, PegasusParquetChunkReader}
import org.apache.spark.internal.Logging
import org.apache.spark.sql.execution.datasources.parquet.{ParquetReadSupport, PegasusVectorizedColumnReader}
import org.apache.spark.sql.execution.vectorized.{OnHeapColumnVector, WritableColumnVector}
import org.apache.spark.sql.types.StructType
import org.apache.spark.sql.vectorized.{ArrowColumnVector, ColumnVector, ColumnarBatch}

class PegasusPartitionReader(configuration: Configuration,
                             ticket: Ticket,
                             location: Location,
                             username: String,
                             password: String,
                             readDataSchema: StructType) extends Logging {

  private val clientFactory = new PegasusClientFactory(
    location, null, null)

  val localIPList:util.ArrayList[String] = getLocalList

  val client = clientFactory.apply
  //TODO make useFileBatch configurable
  private var useFileBatch = true
  var dataReader: DataReader = null
  if(isLocal(location)) {
    dataReader = new LocalDataReader(client, ticket)
  } else {
    dataReader = new GrpcDataReader(client, ticket)
  }


  //TODO consider metadata reading performance

  logInfo("ticket.getPartitionIdentity.toString " + new String(ticket.getPartitionIdentity))
  lazy val footer =  ParquetFileReader.readFooter(configuration, new Path(new String(ticket.getPartitionIdentity)), NO_FILTER)
  val parquetFileSchema = footer.getFileMetaData.getSchema
  val parquetRequestedSchema = ParquetReadSupport.clipParquetSchema(parquetFileSchema,
    readDataSchema, false)

  val blocks = footer.getBlocks
  private var currentBlock = 0

  val reader = new PegasusParquetChunkReader(configuration, new Path(new String(ticket.getPartitionIdentity)), footer, parquetRequestedSchema.getColumns)
  reader.setRequestedSchema(parquetRequestedSchema)

  def next: Boolean = {
//    logInfo("partition schema: " + stream.getRoot.getSchema)
    try {
      dataReader.next()
    } catch {
      case e: Exception =>
        throw new RuntimeException(e)
    }
  }

  def get: ColumnarBatch = {
    try {
      if(useFileBatch) {
        getBatchFromFileBatch
      } else {
        getBatchFromArrowVector
      }
    } catch {
      case e: Exception =>
        throw new RuntimeException(e)
    }
  }

  def close = {
    if (dataReader != null) {
      dataReader.close()
    }
    if (clientFactory != null) {
      clientFactory.close()
    }
    if (client != null) {
      client.close()
    }
  }

  def isLocal(location: Location): Boolean = {
    val host = location.getUri.getHost
    localIPList.contains(host)
//    false
  }

  def getBatchFromFileBatch: ColumnarBatch = {
    val num = dataReader.getBatchCount
    val columnVectors = OnHeapColumnVector.allocateColumns(num, readDataSchema)
    val pages = reader.getRowGroup(dataReader.get)
    if (pages == null) throw new IOException("expecting more rows but reached last block")
    val columnDescriptors = parquetRequestedSchema.getColumns
    val types = parquetRequestedSchema.asGroupType.getFields
    for (i <- 0 until columnDescriptors.size()) {
      // TODO consider null column
      val page = pages.getPageReader(columnDescriptors.get(i))
      val columnReader = new PegasusVectorizedColumnReader(columnDescriptors.get(i), types.get(i).getOriginalType(),
        page, null);
      columnReader.readBatch(num, columnVectors(i));
    }
    val batch = new ColumnarBatch(columnVectors.map { vector =>
      vector.asInstanceOf[WritableColumnVector]
    })
    batch.setNumRows(num)
    batch
  }

  def getBatchFromArrowVector: ColumnarBatch = {
    val columns: Array[ColumnVector] = dataReader.asInstanceOf[GrpcDataReader].getColumns
    val batch = new ColumnarBatch(columns)
    batch.setNumRows(dataReader.getBatchCount)
    batch
  }

  def getLocalList: util.ArrayList[String] = {

    val localIPList = new util.ArrayList[String]
    try {
      var netInterfaces = NetworkInterface.getNetworkInterfaces
      var ip = null
      while (netInterfaces.hasMoreElements) {
        val ni = netInterfaces.nextElement
        val addresses = ni.getInetAddresses
        while (addresses.hasMoreElements) {
          val ip = addresses.nextElement
          if (ip.getHostAddress.indexOf(':') == -1) {
            localIPList.add(ip.getHostAddress)
          }
        }
      }
    } catch {
      case e: Exception => throw(e)
    }
    localIPList
  }

}
