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
package org.apache.parquet.hadoop.pegasus;

import java.io.Closeable;
import java.io.IOException;
import java.util.List;

import org.apache.hadoop.conf.Configuration;
import org.apache.parquet.column.page.PageReadStore;
import org.apache.parquet.hadoop.metadata.BlockMetaData;
import org.apache.parquet.hadoop.metadata.ParquetMetadata;
import org.apache.parquet.schema.MessageType;

import org.apache.parquet.hadoop.PegasusParquetFileReader;

public class ParquetReader implements Closeable {
  private PegasusParquetFileReader reader;
  private int currentBlock = 0;

  private ParquetReader(PegasusParquetFileReader reader) {
    this.reader = reader;
  }

  public static ParquetReader open(Configuration conf, ParquetMetadata footer)
          throws IOException {
    return new ParquetReader(new PegasusParquetFileReader(conf, footer));
  }

  public PageReadStore readNextRowGroup() throws IOException {
    PageReadStore pageReadStore = this.reader.readNextRowGroup();
    currentBlock ++;
    return pageReadStore;
  }

  public void setRequestedSchema(MessageType projection) {
    this.reader.setRequestedSchema(projection);
  }

  public List<BlockMetaData> getRowGroups() {
    return this.reader.getRowGroups();
  }

  @Override
  public void close() throws IOException {
    this.reader.close();
  }

  public ParquetMetadata getFooter() {
    return this.reader.getFooter();
  }
}
