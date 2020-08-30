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

import java.io.IOException
import org.apache.spark._

private[spark] class PartitionStatus {
  def getLen() : Long = 0
}

private[spark] class PegasusSystem(master: String, conf: SparkConf) {
  
  def createPartition(path: PegasusPath): PegasusDataOutputStream = {
    // TO DO
    new PegasusDataOutputStream()
  }

  def openPartition(path: PegasusPath,
    startReduceId: Int, endReduceId: Int) : PegasusDataInputStream = {
    // TO DO
    new PegasusDataInputStream()
  }
  
  def openPartition(path: PegasusPath) : PegasusDataInputStream = {
    // TO DO: also remeber to check the call for how many columns it really write
    new PegasusDataInputStream()
  }
  
  @throws(classOf[IOException])
  def deletePartition(path: PegasusPath) : Boolean = {
    // TO DO
    true
  }
  
  def existsPartition(path: PegasusPath) : Boolean = {
    // TO DO
    true
  }
  
  def renamePartition(path: PegasusPath, newPath: PegasusPath) : Boolean = {
    // TO DO: can only rename in the same dataset with a different partition name
    false
  }
  
  def getPartitionStatus(path: PegasusPath) : PartitionStatus = {
    // TO DO
    new PartitionStatus()
  }
  
}
