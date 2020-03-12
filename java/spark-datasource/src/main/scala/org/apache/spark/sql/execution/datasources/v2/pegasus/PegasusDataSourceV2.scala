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
import scala.collection.JavaConverters._

import com.fasterxml.jackson.databind.ObjectMapper
import org.apache.hadoop.fs.Path
import org.apache.spark.sql.SparkSession
import org.apache.spark.sql.connector.catalog.{Table, TableProvider}
import org.apache.spark.sql.connector.expressions.Transform
import org.apache.spark.sql.sources.DataSourceRegister
import org.apache.spark.sql.types.StructType
import org.apache.spark.sql.util.CaseInsensitiveStringMap
import org.apache.spark.util.Utils

class PegasusDataSourceV2 extends TableProvider with DataSourceRegister {

  def shortName(): String = "pegasus"

  lazy val sparkSession = SparkSession.active

  private var t: Table = null

  override def getTable(
      schema: StructType,
      partitioning: Array[Transform],
      properties: util.Map[String, String]): Table = {
    // If the table is already loaded during schema inference, return it directly.
    if (t != null) {
      t
    } else {
      getTable(new CaseInsensitiveStringMap(properties), schema)
    }
  }

  def getTable(options: CaseInsensitiveStringMap): Table = {
    val paths = getPaths(options)
    val tableName = getTableName(paths)
    PegasusTable(tableName, sparkSession, options, paths, None)
  }

  def getTable(options: CaseInsensitiveStringMap, schema: StructType): Table = {
    val paths = getPaths(options)
    val tableName = getTableName(paths)
    PegasusTable(tableName, sparkSession, options, paths, Some(schema))
  }

  protected def getPaths(map: CaseInsensitiveStringMap): Seq[String] = {
    val objectMapper = new ObjectMapper()
    val paths = Option(map.get("paths")).map { pathStr =>
      objectMapper.readValue(pathStr, classOf[Array[String]]).toSeq
    }.getOrElse(Seq.empty)
    paths ++ Option(map.get("path")).toSeq ++ Option(map.get("location")).toSeq
  }

  protected def getTableName(paths: Seq[String]): String = {
    val name = shortName() + " " + paths.map(qualifiedPathName).mkString(",")
    Utils.redact(sparkSession.sessionState.conf.stringRedactionPattern, name)
  }

  private def qualifiedPathName(path: String): String = {
    val hdfsPath = new Path(path)
    val fs = hdfsPath.getFileSystem(sparkSession.sessionState.newHadoopConf())
    hdfsPath.makeQualified(fs.getUri, fs.getWorkingDirectory).toString
  }

  override def inferSchema(options: CaseInsensitiveStringMap): StructType = {
    if (t == null) t = getTable(options)
    t.schema()
  }
}

