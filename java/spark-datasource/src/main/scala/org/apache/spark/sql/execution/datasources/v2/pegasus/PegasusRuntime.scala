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

import java.util.concurrent.{ConcurrentHashMap, ConcurrentMap}

import org.apache.parquet.hadoop.metadata.ParquetMetadata
import org.apache.spark.{SparkConf, SparkContext, SparkEnv}
import org.apache.spark.util.Utils

object PegasusRuntime {

  private var concurrentMap:ConcurrentMap[String, ParquetMetadata] = null
  /**
    * user transparent initialization
    */
  def getOrCreate: ConcurrentMap[String, ParquetMetadata] = if (concurrentMap == null) init() else concurrentMap


  def init():ConcurrentMap[String, ParquetMetadata] = {
    concurrentMap = new ConcurrentHashMap[String, ParquetMetadata]()
    concurrentMap
  }

}
