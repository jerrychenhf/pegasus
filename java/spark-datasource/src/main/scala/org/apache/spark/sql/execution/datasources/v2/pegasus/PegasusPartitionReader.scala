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

import scala.collection.JavaConverters._
import org.apache.pegasus.rpc.FlightClient
import org.apache.pegasus.rpc.Location
import org.apache.pegasus.rpc.Ticket
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

  val client = clientFactory.apply
  private val stream = client.getStream(ticket)
  //TODO make useFileBatch configurable
  private var useFileBatch = false


  //TODO consider metadata reading performance
  lazy val footer =  ParquetFileReader.readFooter(configuration, new Path(ticket.getPartitionIdentity.toString), NO_FILTER)
  val parquetFileSchema = footer.getFileMetaData.getSchema
  val parquetRequestedSchema = ParquetReadSupport.clipParquetSchema(parquetFileSchema,
    readDataSchema, false)

  val blocks = footer.getBlocks
  private var currentBlock = 0

  val reader = new PegasusParquetChunkReader(configuration, footer, parquetRequestedSchema.getColumns)
  reader.setRequestedSchema(parquetRequestedSchema)

  def next: Boolean = {
    logInfo("partition schema: " + stream.getRoot.getSchema)
    try {
      stream.next()
    } catch {
      case e: Exception =>
        throw new RuntimeException(e)
    }
  }

  def get: ColumnarBatch = {
    try {
      if(useFileBatch) {
        val num = stream.getRoot().getRowCount()
        val columnVectors = OnHeapColumnVector.allocateColumns(num, readDataSchema)
        val pages = reader.getRowGroup(stream.getRoot().getFieldVectors())
        if (pages == null) throw new IOException("expecting more rows but reached last block")
        val columnDescriptors = parquetRequestedSchema.getColumns
        val types = parquetRequestedSchema.asGroupType.getFields
        for (i <- 0 until columnDescriptors.size()) {
          // TODO consider null column
          val columnReader = new PegasusVectorizedColumnReader(columnDescriptors.get(i), types.get(i).getOriginalType(),
            pages.getPageReader(columnDescriptors.get(i)), null);
          columnReader.readBatch(num, columnVectors(i));
        }
        new ColumnarBatch(columnVectors.map { vector =>
          vector.asInstanceOf[WritableColumnVector]
        })
      } else {
        val columns: Array[ColumnVector] = stream.getRoot().getFieldVectors().asScala.map { vector =>
          new ArrowColumnVector(vector).asInstanceOf[ColumnVector]
        }.toArray
        val batch = new ColumnarBatch(columns)
        batch.setNumRows(stream.getRoot().getRowCount())
        batch
      }
    } catch {
      case e: Exception =>
        throw new RuntimeException(e)
    }
  }

  def close = {
    if (stream != null) {
      stream.close()
    }
    if (clientFactory != null) {
      clientFactory.close()
    }
    if (client != null) {
      client.close()
    }
  }
}
