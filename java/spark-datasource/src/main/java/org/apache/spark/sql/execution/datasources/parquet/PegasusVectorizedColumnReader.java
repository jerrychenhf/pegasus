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

package org.apache.spark.sql.execution.datasources.parquet;

import java.io.IOException;
import java.time.ZoneId;

import org.apache.parquet.column.ColumnDescriptor;
import org.apache.parquet.column.page.PageReader;
import org.apache.parquet.schema.OriginalType;

import org.apache.spark.sql.execution.vectorized.WritableColumnVector;

/**
 * Add skip values ability to VectorizedColumnReader, skip method refer to
 * read method of VectorizedColumnReader.
 */
public class PegasusVectorizedColumnReader extends VectorizedColumnReader {

  public PegasusVectorizedColumnReader(
      ColumnDescriptor descriptor,
      OriginalType originalType,
      PageReader pageReader,
      ZoneId convertTz)
      throws IOException {
    super(descriptor, originalType, pageReader, convertTz);
  }
  
  /**
   * Reads `total` values from this columnReader into column.
   */
  public void readBatch(int total, WritableColumnVector column) throws IOException {
  	super.readBatch(total, column);
  }
}
