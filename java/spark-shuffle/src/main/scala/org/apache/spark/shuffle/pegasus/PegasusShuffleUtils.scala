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

import java.util.UUID

import org.apache.spark.SparkEnv
import org.apache.spark.executor.ShuffleWriteMetrics
import org.apache.spark.serializer.{SerializerInstance, SerializerManager}
import org.apache.spark.shuffle.ShuffleWriteMetricsReporter
import org.apache.spark.storage.{BlockId, TempLocalBlockId, TempShuffleBlockId}
import org.apache.spark.shuffle.pegasus.storage.PegasusPath

object PegasusShuffleUtils {

  val env = SparkEnv.get

  private def getPartitionPath(blockId: BlockId, dirUri: String): PegasusPath = {
    new PegasusPath(s"${dirUri}.${blockId.name}")
  }

  /**
   * Something like [[org.apache.spark.storage.DiskBlockManager.createTempShuffleBlock()]], instead
   * returning Path
   */
  private[pegasus] def createTempShuffleBlock(dirUri: String): (TempShuffleBlockId, PegasusPath) = {
    var blockId = new TempShuffleBlockId(UUID.randomUUID())
    val tmpPartitionPath = getPartitionPath(blockId, dirUri)
    val ps = PegasusShuffleManager.getPegasusSystem
    while (ps.existsPartition(tmpPartitionPath)) {
      blockId = new TempShuffleBlockId(UUID.randomUUID())
    }
    (blockId, tmpPartitionPath)
  }

  /**
    * Something like [[org.apache.spark.storage.DiskBlockManager.createTempLocalBlock()]], instead
    * returning Path
    */
  private[pegasus] def createTempLocalBlock(dirUri: String): (TempLocalBlockId, PegasusPath) = {
    var blockId = new TempLocalBlockId(UUID.randomUUID())
    val tmpPartitionPath = getPartitionPath(blockId, dirUri)
    val ps = PegasusShuffleManager.getPegasusSystem
    while (ps.existsPartition(tmpPartitionPath)) {
      blockId = new TempLocalBlockId(UUID.randomUUID())
    }
    (blockId, tmpPartitionPath)
  }

  /**
   * Something like [[org.apache.spark.storage.BlockManager.getDiskWriter()]], instead returning
   * a PegasusBlockObjectWriter
   */
  def getPegasusWriter(
      blockId: BlockId,
      path: PegasusPath,
      serializerManager: SerializerManager,
      serializerInstance: SerializerInstance,
      bufferSize: Int,
      writeMetrics: ShuffleWriteMetricsReporter): PegasusBlockObjectWriter = {
    val syncWrites = false // env.blockManager.conf.getBoolean("spark.shuffle.sync", false)
    new PegasusBlockObjectWriter(path, serializerManager, serializerInstance, bufferSize,
      syncWrites, writeMetrics, blockId)
  }

}
