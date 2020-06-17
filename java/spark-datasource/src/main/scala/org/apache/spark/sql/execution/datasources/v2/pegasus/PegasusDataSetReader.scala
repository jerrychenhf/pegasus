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

import java.util.Locale

import scala.collection.JavaConverters._
import org.apache.pegasus.rpc.{FlightDescriptor, FlightInfo, Location, SchemaResult}
import org.apache.spark.internal.Logging
import org.apache.spark.sql.SparkSession
import org.apache.spark.sql.internal.PegasusConf

class PegasusDataSetReader(
    sparkSession: SparkSession,
    paths: Seq[String],
    options: Map[String, String]) extends AutoCloseable with Logging  {

  private val plannerHost = sparkSession.conf.get(
    PegasusConf.PLANNER_HOST.key, PegasusConf.PLANNER_HOST.defaultValue.get)
  private val plannerPort = sparkSession.conf.get(
    PegasusConf.PLANNER_PORT.key, PegasusConf.PLANNER_PORT.defaultValue.get)

//  private val location = Location.forGrpcInsecure(plannerHost, plannerPort.toInt)
  private val location = Location.forGrpcDomainSocket("/tmp/planner")

  private val userName = sparkSession.conf.get(
    PegasusConf.USERNAME.key, PegasusConf.USERNAME.defaultValue.get)
  private val passWord = sparkSession.conf.get(
    PegasusConf.PASSWORD.key, PegasusConf.PASSWORD.defaultValue.get)

  private val clientFactory = new PegasusClientFactory(
      location, userName, passWord)
  private val client = clientFactory.apply


  private val fileFormat = options.getOrElse("provider",
    FlightDescriptor.FILE_FORMAT_PARQUET).toUpperCase(Locale.ROOT)
  private val properties: Map[String, String] = Seq(
    FlightDescriptor.CATALOG_PROVIDER -> FlightDescriptor.CATALOG_PROVIDER_SPARK,
    FlightDescriptor.FILE_FORMAT -> fileFormat,
    FlightDescriptor.TABLE_LOCATION -> paths(0)).toMap

  def getDataSet(): FlightInfo = {
    val descriptor: FlightDescriptor = FlightDescriptor.path(paths.asJava, (properties ++ options).asJava)
    client.getInfo(descriptor)
  }

  def getSchema(): SchemaResult = {
    val descriptor = FlightDescriptor.path(paths.asJava, (properties ++ options).asJava)
    client.getSchema(descriptor)
  }

  @throws[Exception]
  override def close(): Unit = {
    if (clientFactory != null) {
      clientFactory.close()
    }
    if (client != null) {
      client.close()
    }
  }
}
