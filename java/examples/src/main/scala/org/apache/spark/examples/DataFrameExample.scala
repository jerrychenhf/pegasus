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

import org.apache.spark
import org.apache.spark.SparkConf
import org.apache.spark.internal.config.UI.UI_ENABLED
import org.apache.spark.sql.SparkSession
import org.apache.spark.sql.internal.SQLConf
import org.apache.spark.sql.functions._

object DataFrameExample {

  case class Person(name: String, age: Long)

  def main(args: Array[String]): Unit = {

    val conf = new SparkConf()
      .setAppName("pegasusTest")
      .set("spark.master", "local[1]")
      .set("spark.driver.memory", "3g")
      .set("spark.executor.memory", "3g")
      .set("spark.testing.memory", "10240000000")
//      .set("spark.planner.host", "bdpe822n3")

    val sparkSession = SparkSession.builder.config(conf).getOrCreate()
    val sqlContext = sparkSession.sqlContext
    sparkSession.sparkContext.getConf.getAll.foreach(println)

    val reader = sqlContext.read.format("pegasus")

    val path = "hdfs://10.239.47.55:9000/genData10/customer"
//    val path = "hdfs://10.239.47.55:9000/genData2/income_band"
    val df = reader.option("format", "PARQUET").load(path)
    val df1 = df.select("c_customer_sk")
    df1.printSchema()
    println("table first row: " + df1.head().toString())

    println("==================================================================")
    println("Sleep 7 seconds ...")
    println("==================================================================")
    Thread.sleep(7000L)

    val df_ = reader.option("format", "PARQUET").load(path)
    val df2 = df_.select("c_customer_sk")
    df2.printSchema()
    println("table first row: " + df2.head().toString())

    sparkSession.stop()
  }

}
