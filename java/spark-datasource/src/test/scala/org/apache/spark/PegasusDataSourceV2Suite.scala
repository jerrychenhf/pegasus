/*
 * Copyright (C) 2019 Ryan Murray
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package org.apache.spark

import org.apache.spark.SparkConf
import org.apache.spark.sql.SparkSession
import org.apache.spark.sql.execution.datasources.v2.pegasus.PegasusSparkContext

class PegasusDataSourceV2Suite extends PegasusFunSuite  {

  test("test read table from hdfs") {

    val conf = new SparkConf()
      .setAppName("pegasusTest")
      .setMaster("local[*]")
      .set("spark.driver.allowMultipleContexts", "true")
      .set("planner.host", "localhost")
      .set("planner.port", "30001")
//    val sc = new SparkContext(conf)

    val sparkSession = SparkSession.builder
      .master("local")
      .appName("pegasusTest")
      .config("spark.driver.allowMultipleContexts", "true")
      .config("planner.host", "localhost")
      .config("planner.port", "30001")
      .getOrCreate()

    val psc = new PegasusSparkContext(sparkSession, conf)
    val count = psc.read("test.table").count
//    Assert.assertEquals(20, count)
  }
}
