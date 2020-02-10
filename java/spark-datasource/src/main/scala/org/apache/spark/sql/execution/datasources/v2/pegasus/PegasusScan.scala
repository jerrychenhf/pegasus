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

import org.apache.commons.lang3.StringUtils
import org.apache.hadoop.conf.Configuration
import org.apache.hadoop.fs.Path
import org.apache.spark.internal.Logging
import org.apache.spark.sql.SparkSession
import org.apache.spark.sql.connector.read.{Batch, InputPartition, PartitionReaderFactory, Scan}
import org.apache.spark.sql.types.{Metadata, StructField, StructType}
import org.apache.spark.sql.util.CaseInsensitiveStringMap
import org.apache.spark.util.Utils

import scala.collection.JavaConverters._
import scala.collection.mutable.ArrayBuffer

case class PegasusScan(
    sparkSession: SparkSession,
    hadoopConf: Configuration,
    paths: Seq[String],
    options: CaseInsensitiveStringMap)
  extends Scan with Batch with Logging {

  /**
    * Returns whether a file with `path` could be split or not.
    */
  def isSplitable(path: Path): Boolean = true

  /**
    * Returns the required data schema
    */
  def readDataSchema: StructType = StructType(Seq())

  override def createReaderFactory(): PartitionReaderFactory = {

    PegasusPartitionReaderFactory(options, sparkSession.sessionState.conf, paths)
  }

  override def equals(obj: Any): Boolean = obj match {
    case p: PegasusScan =>
      super.equals(p) && options == p.options && paths == p.paths

    case _ => false
  }

  override def hashCode(): Int = getClass.hashCode()

  protected def seqToString(seq: Seq[Any]): String = seq.mkString("[", ", ", "]")

  override def description(): String = {
    val locationDesc = paths.mkString("[", ", ", "]")
    val metadata: Map[String, String] = Map(
      "ReadSchema" -> readDataSchema.catalogString,
      "Location" -> locationDesc)
    val metadataStr = metadata.toSeq.sorted.map {
      case (key, value) =>
        val redactedValue =
          Utils.redact(sparkSession.sessionState.conf.stringRedactionPattern, value)
        key + ": " + StringUtils.abbreviate(redactedValue, 100)
    }.mkString(", ")
    s"${this.getClass.getSimpleName} $metadataStr"
  }

  protected def partitions: Seq[PegasusPartition] = {

    val pegasusDataReader = new PegasusDataSetReader(options, paths)
    val info = pegasusDataReader.getDataSet()
    val partitions = new ArrayBuffer[PegasusPartition]
    val endpoints = info.getEndpoints.asScala
    var index = 0
    while (index < endpoints.length) {
      val pegasusPartition = PegasusPartition(index, endpoints(index))
      partitions += pegasusPartition
      index += 1
    }
    partitions
  }

  override def planInputPartitions(): Array[InputPartition] = {
    partitions.toArray
  }

  override def toBatch: Batch = this

  override def readSchema(): StructType =
    StructType(readDataSchema.fields)

}
