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

package org.apache.spark.sql.execution.datasources.v2.pegasus

import org.apache.spark.SparkConf
import org.apache.spark.sql.{Dataset, Row, SparkSession}

class PegasusSparkContext(sparkSession: SparkSession, conf: SparkConf) {

  val sqlContext = sparkSession.sqlContext
  val reader = sqlContext.read.format("pegasus")

  def read(s: String): Dataset[Row] = {
    reader.option("port", conf.get("planner.port").toInt)
      .option("host", conf.get("planner.host"))
      .option("parallel", false).load(s)
  }

}
