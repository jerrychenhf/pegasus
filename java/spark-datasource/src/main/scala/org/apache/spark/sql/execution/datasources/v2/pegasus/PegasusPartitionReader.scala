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
import org.apache.parquet.hadoop.metadata.{BlockMetaData, ParquetMetadata}
import org.apache.parquet.hadoop.{ParquetFileReader, PegasusParquetChunkReader}
import org.apache.spark.internal.Logging
import org.apache.spark.memory.MemoryMode
import org.apache.spark.sql.catalyst.InternalRow
import org.apache.spark.sql.execution.datasources.parquet.{ParquetReadSupport, PegasusVectorizedColumnReader, VectorizedColumnReader}
import org.apache.spark.sql.execution.vectorized.{ColumnVectorUtils, OffHeapColumnVector, OnHeapColumnVector, WritableColumnVector}
import org.apache.spark.sql.internal.PegasusConf
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

  val useFileBatch  = configuration.getBoolean(PegasusConf.FILEBATCH_ENABLE.key, true)
  val client = clientFactory.apply(useFileBatch)
  var dataReader: DataReader = null
  val useZeroCopy  = configuration.getBoolean(PegasusConf.ZEROCOPY_ENABLE.key, true)
  if(useZeroCopy && isLocal(location)) {
    dataReader = new LocalDataReader(client, ticket)
  } else {
    dataReader = new GrpcDataReader(client, ticket)
  }

  //TODO consider metadata reading performance

  logInfo("ticket.getPartitionIdentity.toString " + new String(ticket.getPartitionIdentity))

  var footer: ParquetMetadata = null
  val strPath = new String(ticket.getPartitionIdentity)
  if (PegasusRuntime.getOrCreate.containsKey(strPath)) {
    footer = PegasusRuntime.getOrCreate.get(strPath)
//    println("contain:" + strPath)
  } else {
//    println("not contain:" + strPath)
    footer = ParquetFileReader.readFooter(configuration, new Path(new String(ticket.getPartitionIdentity)), NO_FILTER)
//    val map = PegasusRuntime.getOrCreate
    PegasusRuntime.getOrCreate.put(strPath, footer)
  }


  val parquetFileSchema = footer.getFileMetaData.getSchema
  val parquetRequestedSchema = ParquetReadSupport.clipParquetSchema(parquetFileSchema,
    readDataSchema, false)

  val blocks = footer.getBlocks
  private var currentBlock = 0

  var reader = new PegasusParquetChunkReader(configuration, new Path(new String(ticket.getPartitionIdentity)), footer, parquetRequestedSchema.getColumns)
  reader.setRequestedSchema(parquetRequestedSchema)
  val columnDescriptors = parquetRequestedSchema.getColumns


  val capacity = 4096
  var columnReaders = new Array[PegasusVectorizedColumnReader](columnDescriptors.size)
  var currentRowGroupSize = 0
  val columnVectors = OnHeapColumnVector.allocateColumns(capacity, readDataSchema)
  var columnarBatch =  new ColumnarBatch(columnVectors.map { vector =>
    vector.asInstanceOf[WritableColumnVector]
  })
  var totalRowCount:Long = 0
  var rowsReturned = 0
  var totalCountLoadedSoFar = 0


//  for (block: BlockMetaData <- blocks) {
//    totalRowCount += block.getRowCount
//  }

  for(i <- 0 until blocks.size()) {
    totalRowCount += blocks.get(i).getRowCount
  }






  def next: Boolean = {
//    logInfo("partition schema: " + stream.getRoot.getSchema)
    if(useFileBatch) {
      for (vector <- columnVectors) {
        vector.reset();
      }
      columnarBatch.setNumRows(0);
      if (rowsReturned >= totalRowCount) return false
      setNextColumnReaders
      true
    } else {
      dataReader.next()
    }
  }

  def get: ColumnarBatch = {
    try {
      if(useFileBatch) {
//        getBatchFromFileBatch
        getColumnarBatch
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
    if (reader != null) {
      reader.close()
      reader = null
    }
    if (columnarBatch != null) {
      columnarBatch.close()
      columnarBatch = null
    }
  }

  def setNextColumnReaders: Unit = {
    if (rowsReturned != totalCountLoadedSoFar) return
    dataReader.next()
    val currentRowGroupSize = dataReader.getBatchCount
    val pages = reader.getRowGroup(dataReader.get)
    if (pages == null) throw new IOException("expecting more rows but reached last block")
    val types = parquetRequestedSchema.asGroupType.getFields
    for (i <- 0 until columnReaders.length) {
      val page = pages.getPageReader(columnDescriptors.get(i))
      columnReaders(i) = new PegasusVectorizedColumnReader(columnDescriptors.get(i), types.get(i).getOriginalType(),
        page, null);
    }
    totalCountLoadedSoFar += currentRowGroupSize
  }

  def getColumnarBatch: ColumnarBatch = {
    val num = Math.min(capacity.toLong, totalCountLoadedSoFar - rowsReturned).toInt
    for (i <- 0 until columnReaders.length) {
      columnReaders(i).readBatch(num, columnVectors(i))
    }
    rowsReturned += num
    columnarBatch.setNumRows(num)
    columnarBatch
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
//    val columnDescriptors = parquetRequestedSchema.getColumns
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
