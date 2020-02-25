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

import org.apache.hadoop.fs.{FileSystem, Path}
import org.apache.parquet.Log
import org.apache.spark.SparkConf
import org.apache.spark.sql.SparkSession
import org.apache.spark.sql.execution.datasources.parquet.ParquetFileFormat
import org.apache.spark.sql.execution.datasources.v2.pegasus.PegasusSparkContext
import org.junit.Assert

class PegasusDataSourceV2Suite extends PegasusFunSuite  {

  test("test read table from hdfs") {

    val sparkSession = SparkSession.builder
      .master("local")
      .appName("pegasusTest")
      .config("spark.driver.allowMultipleContexts", "true")
      .getOrCreate()

    val sqlContext = sparkSession.sqlContext
    val reader = sqlContext.read.format("pegasus")


    val testAppender = new LogAppender("test pegasus")
    withLogAppender(testAppender) {
      withTempDir { dir =>
        val basePath = dir.getCanonicalPath
        val path1 = new Path(basePath, "first")
        sparkSession.range(1, 3)
          .toDF("c1")
          .coalesce(2)
          .write.parquet(path1.toString)

        val count = reader.option("planner.port", "30001")
          .option("planner.host", "localhost")
          .load("hdfs://10.239.47.55:9000/genData2/customer").count()

        Assert.assertEquals(30, count)
      }
    }

  }
}
