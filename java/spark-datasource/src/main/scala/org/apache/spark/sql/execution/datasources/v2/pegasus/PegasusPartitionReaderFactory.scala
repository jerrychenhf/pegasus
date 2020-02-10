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

import org.apache.spark.internal.Logging
import org.apache.spark.sql.catalyst.InternalRow
import org.apache.spark.sql.connector.read.{InputPartition, PartitionReader, PartitionReaderFactory}
import org.apache.spark.sql.internal.SQLConf
import org.apache.spark.sql.util.CaseInsensitiveStringMap
import org.apache.spark.sql.vectorized.ColumnarBatch

/**
  * A factory used to create Pegasus Partition readers.
  *
  * @param sqlConf SQL configuration.
  */
case class PegasusPartitionReaderFactory(
    options: CaseInsensitiveStringMap,
    sqlConf: SQLConf,
    paths: Seq[String]) extends PartitionReaderFactory with Logging {

  private val pegasusDataReader = new PegasusDataSetReader(options, paths)

  override def supportColumnarReads(partition: InputPartition): Boolean = true

  override def createColumnarReader(partition: InputPartition): PartitionReader[ColumnarBatch] = {
    assert(partition.isInstanceOf[PegasusPartition])
    val pegasusPartition = partition.asInstanceOf[PegasusPartition]
    val pegasusReader = pegasusDataReader.pegasusPartitionReader(pegasusPartition)

    new PartitionReader[ColumnarBatch] {
      override def next(): Boolean = pegasusReader.next

      override def get(): ColumnarBatch =
        pegasusReader.get

      override def close(): Unit = pegasusReader.close
    }
  }

  override def createReader(partition: InputPartition): PartitionReader[InternalRow] = {
    throw new UnsupportedOperationException
  }
}
