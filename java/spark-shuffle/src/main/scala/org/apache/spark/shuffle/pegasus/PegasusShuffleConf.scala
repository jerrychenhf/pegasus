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

package org.apache.spark.shuffle.pegasus

import org.apache.spark.internal.config.{ConfigBuilder, ConfigEntry}

object PegasusShuffleConf {

  val PEGASUS_MASTER_URI: ConfigEntry[String] =
    ConfigBuilder("spark.shuffle.pegasus.master")
      .doc("The master address of pegasus cluster while persisting shuffle files")
      .stringConf
      .createWithDefault("pegasus://localhost:9001")

  val SHUFFLE_DATA_ROOT: ConfigEntry[String] =
    ConfigBuilder("spark.shuffle.pegasus.dataRoot")
      .doc("Use this as the root directory for shuffle files")
      .stringConf
      .createWithDefault("/shuffle")

  val  PEGASUS_OPTIMIZED_SHUFFLE_ENABLED: ConfigEntry[Boolean] =
    ConfigBuilder("spark.shuffle.pegasus.optimizedPathEnabled")
      .doc("Enable using unsafe-optimized shuffle writer")
      .internal()
      .booleanConf
      .createWithDefault(true)

  val PEGASUS_BYPASS_MERGE_THRESHOLD: ConfigEntry[Int] =
    ConfigBuilder("spark.shuffle.pegasus.bypassMergeThreshold")
      .doc("Remote shuffle manager uses this threshold to decide using bypass-merge(hash-based)" +
        "shuffle or not, a new configuration is introduced(and it's -1 by default) because we" +
        " want to explicitly make disabling hash-based shuffle writer as the default behavior." +
        " When memory is relatively sufficient, using sort-based shuffle writer in pegasus shuffle" +
        " is often more efficient than the hash-based one. Because the bypass-merge shuffle " +
        "writer proceeds I/O of 3x total shuffle size: 1 time for read I/O and 2 times for write" +
        " I/Os, and this can be an even larger overhead under pegasus shuffle, the 3x shuffle size" +
        " is gone through network, arriving at pegasus in memory storage system.")
      .intConf
      .createWithDefault(-1)

  val NUM_CONCURRENT_FETCH: ConfigEntry[Int] =
    ConfigBuilder("spark.shuffle.pegasus.numReadThreads")
      .doc("The maximum number of concurrent reading threads fetching shuffle data blocks")
      .intConf
      .createWithDefault(Runtime.getRuntime.availableProcessors())

  val DATA_FETCH_EAGER_REQUIREMENT: ConfigEntry[Boolean] =
    ConfigBuilder("spark.shuffle.pegasus.eagerRequirementDataFetch")
      .doc("With eager requirement = false, a shuffle block will be counted ready and served for" +
        " compute until all content of the block is put in Spark's local memory. With eager " +
        "requirement = true, a shuffle block will be served to later compute after the bytes " +
        "required is fetched and put in memory")
      .booleanConf
      .createWithDefault(false)

}
