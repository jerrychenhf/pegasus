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

package org.apache.spark.util.collection

import java.util.Comparator

import org.apache.spark.shuffle.pegasus.PegasusBlockObjectWriter

/**
  * NOTE: This is to override Spark 3.0.0's WritablePartitionedIterator: Changing the writeNext
  * interface's args type to PegasusBlockObjectWriter
  *
  * Note: We made several places returning a [[PegasusBlockObjectWriter]]. And we didn't just
  * override [[WritablePartitionedPairCollection]], [[PartitionedPairBuffer]] and
  * [[PartitionedAppendOnlyMap]] in order to let the default local sort shuffle manager still work
  * with the pegasus shuffle package existed
  *
  * A common interface for size-tracking collections of key-value pairs that
  *
  *  - Have an associated partition for each key-value pair.
  *  - Support a memory-efficient sorted iterator
  *  - Support a WritablePartitionedIterator for writing the contents directly as bytes.
  */
private[spark] trait PegasusWritablePartitionedPairCollection[K, V] {
  /**
    * Insert a key-value pair with a partition into the collection
    */
  def insert(partition: Int, key: K, value: V): Unit

  /**
    * Iterate through the data in order of partition ID and then the given comparator. This may
    * destroy the underlying collection.
    */
  def partitionedDestructiveSortedIterator(keyComparator: Option[Comparator[K]])
  : Iterator[((Int, K), V)]

  /**
    * Iterate through the data and write out the elements instead of returning them. Records are
    * returned in order of their partition ID and then the given comparator.
    * This may destroy the underlying collection.
    */
  def destructiveSortedWritablePartitionedIterator(keyComparator: Option[Comparator[K]])
  : PegasusWritablePartitionedIterator = {
    val it = partitionedDestructiveSortedIterator(keyComparator)
    new PegasusWritablePartitionedIterator {
      private[this] var cur = if (it.hasNext) it.next() else null

      def writeNext(writer: PegasusBlockObjectWriter): Unit = {
        writer.write(cur._1._2, cur._2)
        cur = if (it.hasNext) it.next() else null
      }

      def hasNext(): Boolean = cur != null

      def nextPartition(): Int = cur._1._1
    }
  }
}

private[spark] object PegasusWritablePartitionedPairCollection {
  /**
    * A comparator for (Int, K) pairs that orders them by only their partition ID.
    */
  def partitionComparator[K]: Comparator[(Int, K)] = new Comparator[(Int, K)] {
    override def compare(a: (Int, K), b: (Int, K)): Int = {
      a._1 - b._1
    }
  }

  /**
    * A comparator for (Int, K) pairs that orders them both by their partition ID
    * and a key ordering.
    */
  def partitionKeyComparator[K](keyComparator: Comparator[K]): Comparator[(Int, K)] = {
    new Comparator[(Int, K)] {
      override def compare(a: (Int, K), b: (Int, K)): Int = {
        val partitionDiff = a._1 - b._1
        if (partitionDiff != 0) {
          partitionDiff
        } else {
          keyComparator.compare(a._2, b._2)
        }
      }
    }
  }
}

/**
  * Iterator that writes elements to a DiskBlockObjectWriter instead of returning them. Each element
  * has an associated partition.
  */
private[spark] trait PegasusWritablePartitionedIterator {
  def writeNext(writer: PegasusBlockObjectWriter): Unit

  def hasNext(): Boolean

  def nextPartition(): Int
}
