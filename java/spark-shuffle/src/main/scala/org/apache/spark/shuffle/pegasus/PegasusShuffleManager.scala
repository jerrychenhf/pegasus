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

import java.io.IOException
import java.net.URL
import java.util.concurrent.ConcurrentHashMap

import org.apache.spark._
import org.apache.spark.deploy.SparkHadoopUtil
import org.apache.spark.internal.{Logging, config}
import org.apache.spark.shuffle._
import org.apache.spark.shuffle.pegasus.PegasusShuffleManager.{active}
import org.apache.spark.shuffle.sort.SortShuffleManager.canUseBatchFetch
import org.apache.spark.shuffle.sort._
import org.apache.spark.util.collection.OpenHashSet
import org.apache.spark.shuffle.pegasus.storage.PegasusSystem

/**
  * In pegasus shuffle, data is written direclty to pegasus distrubted data system in memory.
  */
private[spark] class PegasusShuffleManager(private val conf: SparkConf) extends ShuffleManager with
    Logging {

  require(conf.get(
    config.SHUFFLE_SERVICE_ENABLED.key, config.SHUFFLE_SERVICE_ENABLED.defaultValueString)
      == "false", "Pegasus shuffle and external shuffle service: they cannot be enabled at the" +
      " same time")

  PegasusShuffleManager.setActive(this)

  logWarning("******** Pegasus Shuffle Manager is used ********")

  if (!conf.getBoolean("spark.shuffle.spill", true)) {
    logWarning(
      "spark.shuffle.spill was set to false, but this configuration is ignored as of Spark 1.6+." +
          " Shuffle will continue to spill to disk when necessary.")
  }

  /**
   * A mapping from shuffle ids to the task ids of mappers producing output for those shuffles.
   */
  private[this] val taskIdMapsForShuffle = new ConcurrentHashMap[Int, OpenHashSet[Long]]()

  override val shuffleBlockResolver = new PegasusShuffleBlockResolver(conf)

  /**
    * Obtains a [[ShuffleHandle]] to pass to tasks.
    */
  override def registerShuffle[K, V, C](
      shuffleId: Int,
      dependency: ShuffleDependency[K, V, C]): ShuffleHandle = {
    if (PegasusShuffleManager.shouldBypassMergeSort(conf, dependency)) {
      // If there are fewer than spark.shuffle.sort.bypassMergeThreshold partitions and we don't
      // need map-side aggregation, then write numPartitions files directly and just concatenate
      // them at the end. This avoids doing serialization and deserialization twice to merge
      // together the spilled files, which would happen with the normal code path. The downside is
      // having multiple files open at a time and thus more memory allocated to buffers.
      new BypassMergeSortShuffleHandle[K, V](
        shuffleId, dependency.asInstanceOf[ShuffleDependency[K, V, V]])
    } else if (PegasusShuffleManager.canUseSerializedShuffle(dependency, conf)) {
      new SerializedShuffleHandle[K, V](
        shuffleId, dependency.asInstanceOf[ShuffleDependency[K, V, V]])
    } else {
      // Otherwise, buffer map outputs in a deserialized form:
      new BaseShuffleHandle(shuffleId, dependency)
    }
  }

  /**
    * Get a reader for a range of reduce partitions (startPartition to endPartition-1, inclusive).
    * Called on executors by reduce tasks.
    */
  override def getReader[K, C](
      handle: ShuffleHandle,
      startPartition: Int,
      endPartition: Int,
      context: TaskContext,
      metrics: ShuffleReadMetricsReporter): ShuffleReader[K, C] = {

    val blocksByAddress = SparkEnv.get.mapOutputTracker.getMapSizesByExecutorId(
      handle.shuffleId, startPartition, endPartition)

    new PegasusShuffleReader(
      handle.asInstanceOf[BaseShuffleHandle[K, _, C]],
      shuffleBlockResolver,
      blocksByAddress,
      context,
      metrics,
      shouldBatchFetch = canUseBatchFetch(startPartition, endPartition, context))
  }

  override def getReaderForRange[K, C](
      handle: ShuffleHandle,
      startMapIndex: Int,
      endMapIndex: Int,
      startPartition: Int,
      endPartition: Int,
      context: TaskContext,
      metrics: ShuffleReadMetricsReporter): ShuffleReader[K, C] = {

    val blocksByAddress = SparkEnv.get.mapOutputTracker.getMapSizesByRange(
      handle.shuffleId, startMapIndex, endMapIndex, startPartition, endPartition)

    new PegasusShuffleReader(
      handle.asInstanceOf[BaseShuffleHandle[K, _, C]],
      shuffleBlockResolver,
      blocksByAddress,
      context,
      metrics,
      shouldBatchFetch = canUseBatchFetch(startPartition, endPartition, context))
  }

  /** Get a writer for a given partition. Called on executors by map tasks. */
  override def getWriter[K, V](
      handle: ShuffleHandle,
      mapId: Long,
      context: TaskContext,
      metrics: ShuffleWriteMetricsReporter): ShuffleWriter[K, V] = {
    val mapTaskIds = taskIdMapsForShuffle.computeIfAbsent(
      handle.shuffleId, _ => new OpenHashSet[Long](16))
    mapTaskIds.synchronized { mapTaskIds.add(context.taskAttemptId()) }
    val env = SparkEnv.get
    handle match {
      case unsafeShuffleHandle: SerializedShuffleHandle[K @unchecked, V @unchecked] =>
        new PegasusUnsafeShuffleWriter(
          env.blockManager,
          shuffleBlockResolver,
          context.taskMemoryManager(),
          unsafeShuffleHandle,
          mapId,
          context,
          env.conf,
          metrics)
      case bypassMergeSortHandle: BypassMergeSortShuffleHandle[K @unchecked, V @unchecked] =>
        new PegasusBypassMergeSortShuffleWriter(
          env.blockManager,
          shuffleBlockResolver,
          bypassMergeSortHandle,
          mapId,
          context,
          env.conf,
          metrics)
      case other: BaseShuffleHandle[K @unchecked, V @unchecked, _] =>
        new PegasusShuffleWriter(shuffleBlockResolver, other, mapId, context)
    }
  }

  /** Remove a shuffle's metadata from the ShuffleManager. */
  override def unregisterShuffle(shuffleId: Int): Boolean = {
    Option(taskIdMapsForShuffle.remove(shuffleId)).foreach { mapTaskIds =>
      mapTaskIds.iterator.foreach { mapTaskId =>
        shuffleBlockResolver.removeShuffleOutput(shuffleId, mapTaskId)
      }
    }
    true
  }

  /** Shut down this ShuffleManager. */
  override def stop(): Unit = {
    shuffleBlockResolver.stop()
  }
}


private[spark] object PegasusShuffleManager extends Logging {

  var active: PegasusShuffleManager = _
  private[pegasus] def setActive(update: PegasusShuffleManager): Unit = active = update

  def getPegasusSystem : PegasusSystem = {
    require(active != null,
      "Active PegasusShuffleManager unassigned.")
    active.shuffleBlockResolver.ps
  }
  
  def getResolver: PegasusShuffleBlockResolver = {
    require(active != null,
      "Active PegasusShuffleManager unassigned! It's probably never constructed")
    active.shuffleBlockResolver
  }

  def getConf: SparkConf = {
    require(active != null,
      "Active PegasusShuffleManager unassigned! It's probably never constructed")
    active.conf
  }

  /**
    * Make the decision also referring to a configuration
    */
  def canUseSerializedShuffle(dependency: ShuffleDependency[_, _, _], conf: SparkConf): Boolean = {
    val optimizedShuffleEnabled = conf.get(PegasusShuffleConf.PEGASUS_OPTIMIZED_SHUFFLE_ENABLED)
    optimizedShuffleEnabled && SortShuffleManager.canUseSerializedShuffle(dependency)
  }

  // This is identical to [[SortShuffleWriter.shouldBypassMergeSort]], except reading from
  // a modified configuration name, due to we need to change the default threshold to -1 in pegasus
  // shuffle
  def shouldBypassMergeSort(conf: SparkConf, dep: ShuffleDependency[_, _, _]): Boolean = {

    // We cannot bypass sorting if we need to do map-side aggregation.
    if (dep.mapSideCombine) {
      false
    } else {
      val bypassMergeThreshold = conf.get(PegasusShuffleConf.PEGASUS_BYPASS_MERGE_THRESHOLD)
      dep.partitioner.numPartitions <= bypassMergeThreshold
    }
  }
}
