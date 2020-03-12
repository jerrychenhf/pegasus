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

import java.util

import org.apache.spark.sql.{AnalysisException, SparkSession}
import org.apache.spark.sql.connector.catalog.{SupportsRead, Table, TableCapability}
import org.apache.spark.sql.types._
import org.apache.spark.sql.util.{ArrowUtils, CaseInsensitiveStringMap}

import scala.collection.JavaConverters._

case class PegasusTable(
                         name: String,
                         sparkSession: SparkSession,
                         options: CaseInsensitiveStringMap,
                         paths: Seq[String],
                         userSpecifiedSchema: Option[StructType])
  extends Table with SupportsRead {

  override def newScanBuilder(options: CaseInsensitiveStringMap): PegasusScanBuilder =
    new PegasusScanBuilder(sparkSession, paths, schema, options)

  override def capabilities(): util.Set[TableCapability] = {
    Set(TableCapability.BATCH_READ, TableCapability.ACCEPT_ANY_SCHEMA).asJava
  }

  override lazy val schema: StructType = {
    val schema = userSpecifiedSchema.map { schema =>
      StructType(schema)
    }.orElse {
      inferSchema()
    }.getOrElse {
      throw new AnalysisException(
        s"Unable to infer schema. It must be specified manually.")
    }
    schema
  }

  def inferSchema(): Option[StructType] = {

    val pegasusDataSetReader = new PegasusDataSetReader(sparkSession, paths, options.asScala.toMap)
    val schema  = {
      try {
        pegasusDataSetReader.getSchema()
      } catch {
        case e: Exception =>
          throw new RuntimeException(e)
      } finally {
        pegasusDataSetReader.close()
      }
    }

    if (schema != null && schema.getSchema != null) {
      Some(ArrowUtils.fromArrowSchema(schema.getSchema))
    } else {
      Some(StructType(Seq()))
    }
  }

}
