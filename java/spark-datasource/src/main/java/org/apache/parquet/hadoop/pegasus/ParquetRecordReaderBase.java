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

import java.io.IOException;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

import org.apache.hadoop.conf.Configuration;
import org.apache.parquet.hadoop.api.InitContext;
import org.apache.parquet.hadoop.api.ReadSupport;
import org.apache.parquet.hadoop.metadata.BlockMetaData;
import org.apache.parquet.hadoop.metadata.ParquetMetadata;
import org.apache.parquet.schema.MessageType;

import org.apache.spark.sql.execution.datasources.parquet.ParquetReadSupport;
import org.apache.spark.sql.types.StructType;
import org.apache.spark.sql.types.StructType$;

public abstract class ParquetRecordReaderBase<T> implements RecordReader<T> {

    /**
     * From ParquetRecordReaderBase.
     */
    protected MessageType fileSchema;
    protected MessageType requestedSchema;
    protected StructType sparkSchema;

    /**
     * The total number of rows this RecordReader will eventually read. The sum of the
     * rows of all the row groups.
     */
    protected long totalRowCount;
    protected ParquetReader reader;

    /**
     *
     * @param footer parquet file footer
     * @param configuration haddoop configuration
     * @throws IOException
     * @throws InterruptedException
     */
    protected void initialize(
        ParquetMetadata footer,
        Configuration configuration) throws IOException, InterruptedException {
      this.fileSchema = footer.getFileMetaData().getSchema();
      Map<String, String> fileMetadata = footer.getFileMetaData().getKeyValueMetaData();
      ReadSupport.ReadContext readContext = new ParquetReadSupport().init(new InitContext(
        configuration, toSetMultiMap(fileMetadata), fileSchema));
      this.requestedSchema = readContext.getRequestedSchema();
      String sparkRequestedSchemaString =
        configuration.get(ParquetReadSupport.SPARK_ROW_REQUESTED_SCHEMA());
      this.sparkSchema = StructType$.MODULE$.fromString(sparkRequestedSchemaString);
      this.reader = ParquetReader.open(configuration, footer);
      this.reader.setRequestedSchema(requestedSchema);
      for (BlockMetaData block : this.reader.getRowGroups()) {
        this.totalRowCount += block.getRowCount();
      }
    }

    @Override
    public void close() throws IOException {
      if (reader != null) {
        reader.close();
        reader = null;
      }
    }
    
    public static <K, V> Map<K, Set<V>> toSetMultiMap(Map<K, V> map) {
      Map<K, Set<V>> setMultiMap = new HashMap<>();
      for (Map.Entry<K, V> entry : map.entrySet()) {
        Set<V> set = new HashSet<>();
        set.add(entry.getValue());
        setMultiMap.put(entry.getKey(), Collections.unmodifiableSet(set));
      }
      return Collections.unmodifiableMap(setMultiMap);
    }
}
