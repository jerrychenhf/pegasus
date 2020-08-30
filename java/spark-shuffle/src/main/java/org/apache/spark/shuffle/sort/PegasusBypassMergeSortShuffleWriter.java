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

package org.apache.spark.shuffle.sort;

import java.io.IOException;

import javax.annotation.Nullable;

import org.apache.spark.shuffle.ShuffleWriteMetricsReporter;
import scala.None$;
import scala.Option;
import scala.Product2;
import scala.Tuple2;
import scala.collection.Iterator;

import com.google.common.annotations.VisibleForTesting;
import com.google.common.io.Closeables;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import org.apache.spark.*;
import org.apache.spark.executor.ShuffleWriteMetrics;
import org.apache.spark.scheduler.MapStatus;
import org.apache.spark.scheduler.MapStatus$;
import org.apache.spark.serializer.Serializer;
import org.apache.spark.serializer.SerializerInstance;
import org.apache.spark.serializer.SerializerManager;
import org.apache.spark.shuffle.ShuffleWriter;
import org.apache.spark.shuffle.pegasus.*;
import org.apache.spark.shuffle.pegasus.storage.*;
import org.apache.spark.storage.BlockId;
import org.apache.spark.storage.BlockManager;
import org.apache.spark.storage.TempShuffleBlockId;
import org.apache.spark.util.Utils;

/**
 * This class implements sort-based shuffle's hash-style shuffle fallback path. This write path
 * writes incoming records to separate files, one file per reduce partition, then concatenates these
 * per-partition files to form a single output file, regions of which are served to reducers.
 * Records are not buffered in memory. It writes output in a format
 * that can be served / consumed via
 * {@link org.apache.spark.shuffle.pegasus.PegasusShuffleBlockResolver}.
 * <p>
 * This write path is inefficient for shuffles with large numbers of reduce partitions because it
 * simultaneously opens separate serializers and file streams for all partitions. As a result,
 * {@link SortShuffleManager} only selects this write path when
 * <ul>
 *    <li>no Ordering is specified,</li>
 *    <li>no Aggregator is specified, and</li>
 *    <li>the number of partitions is less than
 *      <code>spark.shuffle.sort.bypassMergeThreshold</code>.</li>
 * </ul>
 *
 * This code used to be part of {@link org.apache.spark.util.collection.ExternalSorter} but was
 * refactored into its own class in order to reduce code complexity; see SPARK-7855 for details.
 * <p>
 * There have been proposals to completely remove this code path; see SPARK-6026 for details.
 */
public final class PegasusBypassMergeSortShuffleWriter<K, V> extends ShuffleWriter<K, V> {

  private final SerializerManager serializerManager = SparkEnv.get().serializerManager();

  private static final Logger logger = LoggerFactory.getLogger(
      PegasusBypassMergeSortShuffleWriter.class);

  static {
    logger.warn("******** Bypass-Merge-Sort Pegasus Shuffle Writer is used ********");
  }

  private final int fileBufferSize;
  private final boolean transferToEnabled;
  private final int numPartitions;
  private final BlockManager blockManager;
  private final Partitioner partitioner;
  private final ShuffleWriteMetricsReporter writeMetrics;
  private final int shuffleId;
  private final long mapId;
  private final Serializer serializer;
  private final PegasusShuffleBlockResolver shuffleBlockResolver;

  /** Array of file writers, one for each partition */
  private PegasusBlockObjectWriter[] partitionWriters;
  private PegasusDataSegment[] partitionWriterSegments;
  @Nullable private MapStatus mapStatus;
  private long[] partitionLengths;

  /**
   * Are we in the process of stopping? Because map tasks can call stop() with success = true
   * and then call stop() with success = false if they get an exception, we want to make sure
   * we don't try deleting files, etc twice.
   */
  private boolean stopping = false;

  public PegasusBypassMergeSortShuffleWriter(
      BlockManager blockManager,
      PegasusShuffleBlockResolver shuffleBlockResolver,
      BypassMergeSortShuffleHandle<K, V> handle,
      long mapId,
      TaskContext taskContext,
      SparkConf conf,
      ShuffleWriteMetricsReporter metrics) {
    // Use getSizeAsKb (not bytes) to maintain backwards compatibility if no units are provided
    this.fileBufferSize = (int) conf.getSizeAsKb("spark.shuffle.file.buffer", "32k") * 1024;
    this.transferToEnabled = conf.getBoolean("spark.file.transferTo", true);
    this.blockManager = blockManager;
    final ShuffleDependency<K, V, V> dep = handle.dependency();
    this.mapId = mapId;
    this.shuffleId = dep.shuffleId();
    this.partitioner = dep.partitioner();
    this.numPartitions = partitioner.numPartitions();
    this.writeMetrics = metrics;
    this.serializer = dep.serializer();
    this.shuffleBlockResolver = shuffleBlockResolver;
  }

  @Override
  public void write(Iterator<Product2<K, V>> records) throws IOException {
    assert (partitionWriters == null);
    if (!records.hasNext()) {
      partitionLengths = new long[numPartitions];
      //TO DO: handling empty data
      shuffleBlockResolver.commitOutput(shuffleId, mapId, partitionLengths, null);
      mapStatus = MapStatus$.MODULE$.apply(
          PegasusShuffleManager$.MODULE$.getResolver().shuffleServerId(), partitionLengths, mapId);
      return;
    }
    final SerializerInstance serInstance = serializer.newInstance();
    final long openStartTime = System.nanoTime();
    partitionWriters = new PegasusBlockObjectWriter[numPartitions];
    partitionWriterSegments = new PegasusDataSegment[numPartitions];
    for (int i = 0; i < numPartitions; i++) {
      final Tuple2<TempShuffleBlockId, PegasusPath> tempShuffleBlockIdPlusFile =
          shuffleBlockResolver.createTempShuffleBlock();
      final PegasusPath file = tempShuffleBlockIdPlusFile._2();
      final BlockId blockId = tempShuffleBlockIdPlusFile._1();
      partitionWriters[i] =
          PegasusShuffleUtils.getPegasusWriter(
              blockId, file, serializerManager, serInstance, fileBufferSize, writeMetrics);
    }
    // Creating the file to write to and creating a disk writer both involve interacting with
    // the disk, and can take a long time in aggregate when we open many files, so should be
    // included in the shuffle write time.
    writeMetrics.incWriteTime(System.nanoTime() - openStartTime);

    while (records.hasNext()) {
      final Product2<K, V> record = records.next();
      final K key = record._1();
      partitionWriters[partitioner.getPartition(key)].write(key, record._2());
    }

    for (int i = 0; i < numPartitions; i++) {
      final PegasusBlockObjectWriter writer = partitionWriters[i];
      partitionWriterSegments[i] = writer.commitAndGet();
      writer.close();
    }

    // TO DO: handle temp data if there are some failures
    PegasusPath output = shuffleBlockResolver.getShuffleOutput(shuffleId, mapId);
    try {
      partitionLengths = writePartitionedOutput(output);
      shuffleBlockResolver.commitOutput(shuffleId, mapId, partitionLengths, output);
    } finally {
      // TO DO: handle temp data if there are some failures
    }
    mapStatus = MapStatus$.MODULE$.apply(
        PegasusShuffleManager$.MODULE$.getResolver().shuffleServerId(), partitionLengths, mapId);
  }

  @VisibleForTesting
  long[] getPartitionLengths() {
    return partitionLengths;
  }

  /**
   * Concatenate all of the per-partition files into a single combined file.
   *
   * @return array of lengths, in bytes, of each partition of the file (used by map output tracker).
   */
  private long[] writePartitionedOutput(PegasusPath outputFile) throws IOException {
    final PegasusSystem ps = PegasusShuffleManager.getPegasusSystem();
    // Track location of the partition starts in the output file
    final long[] lengths = new long[numPartitions];
    if (partitionWriters == null) {
      // We were passed an empty iterator
      return lengths;
    }

    final PegasusDataOutputStream out = ps.createPartition(outputFile);
    final long writeStartTime = System.nanoTime();
    boolean threwException = true;
    try {
      for (int i = 0; i < numPartitions; i++) {
        final PegasusPath file = partitionWriterSegments[i].path();
        if (ps.existsPartition(file)) {
          final PegasusDataInputStream in = ps.openPartition(file);
          boolean copyThrewException = true;
          try {
            lengths[i] = Utils.copyStream(in, out, false, transferToEnabled);
            copyThrewException = false;
          } finally {
            Closeables.close(in, copyThrewException);
          }
          if (!ps.deletePartition(file)) {
            logger.error("Unable to delete file for partition {}", i);
          }
        }
      }
      threwException = false;
    } finally {
      Closeables.close(out, threwException);
      writeMetrics.incWriteTime(System.nanoTime() - writeStartTime);
    }
    partitionWriters = null;
    return lengths;
  }

  @Override
  public Option<MapStatus> stop(boolean success) {
    if (stopping) {
      return None$.empty();
    } else {
      stopping = true;
      if (success) {
        if (mapStatus == null) {
          throw new IllegalStateException("Cannot call stop(true) without having called write()");
        }
        return Option.apply(mapStatus);
      } else {
        // The map task failed, so delete our output data.
        if (partitionWriters != null) {
          try {
            PegasusSystem ps = null;
            for (PegasusBlockObjectWriter writer : partitionWriters) {
              // This method explicitly does _not_ throw exceptions:
              PegasusPath file = writer.revertPartialWritesAndClose();
              ps = PegasusShuffleManager.getPegasusSystem();
              if (!ps.deletePartition(file)) {
                logger.error("Error while deleting file {}", file.toString());
              }
            }
          } catch (IOException e) {
            e.printStackTrace();
          } finally {
            partitionWriters = null;
          }
        }
        return None$.empty();
      }
    }
  }
}
