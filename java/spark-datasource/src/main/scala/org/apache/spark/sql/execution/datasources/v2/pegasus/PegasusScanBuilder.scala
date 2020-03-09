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

import org.apache.pegasus.rpc.FlightInfo
import org.apache.spark.internal.Logging
import org.apache.spark.sql.SparkSession
import org.apache.spark.sql.connector.read.{Scan, ScanBuilder, SupportsPushDownRequiredColumns}
import org.apache.spark.sql.execution.datasources.PartitioningUtils
import org.apache.spark.sql.types.StructType
import org.apache.spark.sql.util.CaseInsensitiveStringMap

import scala.collection.JavaConverters._

case class PegasusScanBuilder(
                               sparkSession: SparkSession,
                               paths: Seq[String],
                               dataSchema: StructType,
                               options: CaseInsensitiveStringMap) extends ScanBuilder
  with SupportsPushDownRequiredColumns with Logging {

  protected val supportsNestedSchemaPruning = true
  protected var requiredSchema: StructType = dataSchema

  lazy val hadoopConf = {
    val caseSensitiveMap = options.asCaseSensitiveMap.asScala.toMap
    // Hadoop Configurations are case sensitive.
    sparkSession.sessionState.newHadoopConfWithOptions(caseSensitiveMap)
  }

  override def build(): Scan = {
    PegasusScan(sparkSession, hadoopConf, paths, dataSchema, readDataSchema(), options)
  }

  override def pruneColumns(requiredSchema: StructType): Unit = {
    this.requiredSchema = requiredSchema
    logInfo("required schema: " + requiredSchema)
  }

  /**
   * Returns the required data schema
   */
  protected def readDataSchema(): StructType = {
    if (supportsNestedSchemaPruning && requiredSchema.length != 0) {
      requiredSchema
    } else {
      dataSchema
    }
  }

}
