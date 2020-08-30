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

package org.apache.spark.shuffle.pegasus.storage

import java.io.{ByteArrayInputStream, InputStream, IOException}
import java.nio.ByteBuffer
import java.util.concurrent.ConcurrentHashMap

import scala.collection.mutable

import io.netty.buffer.{ByteBuf, Unpooled}
import org.apache.hadoop.fs.{FSDataInputStream, Path}

import org.apache.spark.internal.Logging
import org.apache.spark.network.buffer.ManagedBuffer
import org.apache.spark.network.protocol.{Encodable, Encoders}
import org.apache.spark.network.util.{JavaUtils, LimitedInputStream}
import org.apache.spark.shuffle.pegasus.PegasusShuffleManager

/**
 * Something like [[org.apache.spark.network.buffer.FileSegmentManagedBuffer]], instead we only
 * need createInputStream function, so we don't need a TransportConf field, which is intended to
 * be used in other functions
 */
private[spark] class PegasusShuffleOutputManagedBuffer(
    val path: PegasusPath, val startReduceId: Int, val endReduceId: Int,
    val length: Long, var eagerRequirement: Boolean = false)
    extends ManagedBuffer with Logging {

  import PegasusShuffleOutputManagedBuffer._

  private lazy val dataStream: InputStream = {
    if (length == 0) {
      new ByteArrayInputStream(new Array[Byte](0))
    } else {
      var is: PegasusDataInputStream = null
      is = ps.openPartition(path, startReduceId, endReduceId)
      new LimitedInputStream(is, length)
    }
  }

  private lazy val dataInByteArray: Array[Byte] = {
    if (length == 0) {
      Array.empty[Byte]
    } else {
      var is: PegasusDataInputStream = null
      try {
        is = ps.openPartition(path, startReduceId, endReduceId)
        val array = new Array[Byte](length.toInt)
        is.readFully(array)
        array
      } catch {
        case e: IOException =>
          var errorMessage = "Error in reading " + this
          
          // TO DO: check the size
          /*
          if (is != null) {
            val size = fs.getFileStatus(file).getLen
            errorMessage += " (actual file length " + size + ")"
          }
          */
          throw new IOException(errorMessage, e)
      } finally {
        //TO DO: do clean up
      }
    }
  }

  private[spark] def prepareData(eagerRequirement: Boolean): Unit = {
    this.eagerRequirement = eagerRequirement
    if (! eagerRequirement) {
      dataInByteArray
    }
  }

  override def size(): Long = length

  override def createInputStream(): InputStream = if (eagerRequirement) {
    logInfo("Eagerly requiring this data input stream")
    dataStream
  } else {
    new ByteArrayInputStream(dataInByteArray)
  }

  override def equals(obj: Any): Boolean = {
    if (! obj.isInstanceOf[PegasusShuffleOutputManagedBuffer]) {
      false
    } else {
      val buffer = obj.asInstanceOf[PegasusShuffleOutputManagedBuffer]
      this.path == buffer.path && this.startReduceId == buffer.startReduceId &&
        this.endReduceId == buffer.endReduceId && this.length == buffer.length
    }
  }

  override def hashCode(): Int = super.hashCode()

  override def retain(): ManagedBuffer = this

  override def release(): ManagedBuffer = this

  override def nioByteBuffer(): ByteBuffer = throw new UnsupportedOperationException

  override def convertToNetty(): AnyRef = throw new UnsupportedOperationException
}

private[pegasus] object PegasusShuffleOutputManagedBuffer {
  private val ps = PegasusShuffleManager.getPegasusSystem
  // TO DO: remove?
  private[pegasus] lazy val handleCache =
    new ConcurrentHashMap[Long, mutable.HashMap[Path, FSDataInputStream]]()
}

