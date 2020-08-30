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

import java.io._
import java.nio.{ByteBuffer, LongBuffer}
import java.util.UUID
import java.util.function.Consumer

import scala.collection.mutable
import com.google.common.cache.{CacheBuilder, CacheLoader, Weigher}
import org.apache.spark.{SparkConf, SparkEnv}
import org.apache.spark.internal.Logging
import org.apache.spark.internal.config.BLOCK_MANAGER_PORT
import org.apache.spark.network.BlockTransferService
import org.apache.spark.network.buffer.ManagedBuffer
import org.apache.spark.network.shuffle.ShuffleIndexRecord
import org.apache.spark.network.util.JavaUtils
import org.apache.spark.shuffle.pegasus.storage._
import org.apache.spark.shuffle.ShuffleBlockResolver
import org.apache.spark.storage.{BlockId, ShuffleBlockBatchId, ShuffleBlockId, TempLocalBlockId, TempShuffleBlockId}
import org.apache.spark.util.Utils

/**
  * Create and maintain the shuffle blocks' mapping between logic block and physical file location.
  * It also manages the resource cleaning and temporary files creation,
  * like a [[org.apache.spark.shuffle.IndexShuffleBlockResolver]] ++
  * [[org.apache.spark.storage.DiskBlockManager]]
  *
  */
class PegasusShuffleBlockResolver(conf: SparkConf) extends ShuffleBlockResolver with Logging {

  private val master = conf.get(PegasusShuffleConf.PEGASUS_MASTER_URI)
  private val rootDir = conf.get(PegasusShuffleConf.SHUFFLE_DATA_ROOT)
  // 1. Use lazy evaluation due to at the time this class(and its fields) is initialized,
  // SparkEnv._conf is not yet set
  // 2. conf.getAppId may not always work, because during unit tests we may just new a Resolver
  // instead of getting one from the ShuffleManager referenced by SparkContext
  private lazy val applicationId =
    if (Utils.isTesting) s"test${UUID.randomUUID()}" else conf.getAppId
  private def shuffleDatasetPrefix = s"${rootDir}/${applicationId}_shuffle"
  private def spillDatasetPrefix = s"${rootDir}/${applicationId}_spill"

  // This referenced is shared for all the I/Os with shuffling storage system
  lazy val ps = getPegasusSystem(master)
  
  private def getPegasusSystem(master: String):  PegasusSystem = {
      // TO DO: new and initilize the PegasusSystem for accessing entry to Pegasus
      new PegasusSystem(master, PegasusShuffleManager.getConf)
  }

  private[pegasus] lazy val shuffleServerId = {
      SparkEnv.get.blockManager.blockManagerId
  }

 // TO DO: no need data file, replace with dataset id and partition id
  def getShuffleOutput(shuffleId: Int, mapId: Long): PegasusPath = {
    new PegasusPath(s"${shuffleDatasetPrefix}_${shuffleId}.${mapId}")
  }

  /**
   * It will commit the data to the pegasus system
   *
   * Note: the `lengths` will be updated to match the existing index file if use the existing ones.
   */
  def commitOutput(
      shuffleId: Int,
      mapId: Long,
      lengths: Array[Long],
      path: PegasusPath): Unit = {
   // TO DO: implement pegasus data commit
  }

  def getBlockData(blockId: BlockId, dirs: Option[Array[String]] = None): ManagedBuffer = {
    val (shuffleId, mapId, startReduceId, endReduceId) = blockId match {
      case id: ShuffleBlockId =>
        (id.shuffleId, id.mapId, id.reduceId, id.reduceId + 1)
      case batchId: ShuffleBlockBatchId =>
        (batchId.shuffleId, batchId.mapId, batchId.startReduceId, batchId.endReduceId)
      case _ =>
        throw new IllegalArgumentException("unexpected shuffle block id format: " + blockId)
    }

    //TO DO: get the length of the managed buffer
    new PegasusShuffleOutputManagedBuffer(
      getShuffleOutput(shuffleId, mapId),
      startReduceId, endReduceId, 0)
  }

  /**
    * Remove the shuffle output file
    */
  def removeShuffleOutput(shuffleId: Int, mapId: Long): Unit = {
    // TO DO: implement remove of the shuffle output partition for the map task
  }

  def createTempShuffleBlock(): (TempShuffleBlockId, PegasusPath) = {
    PegasusShuffleUtils.createTempShuffleBlock(spillDatasetPrefix)
  }

  def createTempLocalBlock(): (TempLocalBlockId, PegasusPath) = {
    PegasusShuffleUtils.createTempLocalBlock(spillDatasetPrefix)
  }

  override def stop(): Unit = {
    // TO DO: Clean up all the shuffle data of the application
    // for HDFS, remove the shuffle directory, for pegasus, we remove the dataset
   
   // TO DO: Cleanup the managed buffer if needed
   /* 
    try {
      PegasusShuffleOutputManagedBuffer.handleCache.values().forEach {
        new Consumer[mutable.HashMap[Path, FSDataInputStream]] {
          override def accept(t: mutable.HashMap[Path, FSDataInputStream]): Unit = {
            t.values.foreach(JavaUtils.closeQuietly)
          }
        }
      }
    } catch {
      case e: Exception => logInfo(s"Exception thrown when closing. " +
          s"Caused by: ${e.toString}\n${e.getStackTrace.mkString("\n")}")
    }
    */
  }
}
