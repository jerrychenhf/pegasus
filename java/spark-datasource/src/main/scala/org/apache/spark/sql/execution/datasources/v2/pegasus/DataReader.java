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

package org.apache.spark.sql.execution.datasources.v2.pegasus;

import org.apache.spark.annotation.Evolving;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.List;

/**
 * A logical representation of parquet chunk buffer reader.
 *
 * @since 3.0.0
 */
@Evolving
public interface DataReader {

  /**
   * Proceed to next columnarBatch, returns false if there is no more records.
   *
   * @throws IOException if failure happens during disk/network IO like reading files.
   */
  boolean next() throws IOException;

  /**
   * Return the current columnarBatch. This method should return same value until `next` is called.
   */
  List<ByteBuffer> get();

  /**
   * Return the current columnarBatch count. This method should return same value until `next` is called.
   */
  int getBatchCount();

  /**
   * Release resource or references
   */
  void close();
}
