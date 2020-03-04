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
package org.apache.spark.examples

import java.util.Properties

import org.apache.spark.SparkConf
import org.apache.spark.internal.config.UI.UI_ENABLED
import org.apache.spark.sql.SparkSession

object DataFrameExample {

  case class Person(name: String, age: Long)

  def main(args: Array[String]): Unit = {

    val conf = new SparkConf()
      .setAppName("pegasusTest")
      .set("spark.master", "local[1]")
      .set("spark.driver.memory", "3g")
      .set("spark.executor.memory", "3g")
      .set("spark.testing.memory", "10240000000")

    val sparkSession = SparkSession.builder.config(conf).getOrCreate()

    val sqlContext = sparkSession.sqlContext
    val reader = sqlContext.read.format("pegasus")

//    val path = "hdfs://10.239.47.55:9000/genData2/customer"
    val path = "hdfs://10.239.47.55:9000/genData2/income_band"
    val count = reader
      .option("planner.port", "30001")
      .option("planner.host", "localhost")
      .option("provider", "SPARK")
      .option("table.location", path)
      .option("format", "PARQUET")
      .load(path)
      .count()

    sparkSession.stop()
  }

}
