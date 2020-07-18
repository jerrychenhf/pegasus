/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

package org.apache.parquet.hadoop;

import com.google.common.collect.Maps;
import org.apache.arrow.vector.FieldVector;
import org.apache.hadoop.conf.Configuration;
import org.apache.parquet.column.ColumnDescriptor;
import org.apache.parquet.column.page.PageReadStore;
import org.apache.parquet.hadoop.metadata.BlockMetaData;
import org.apache.parquet.hadoop.metadata.ColumnChunkMetaData;
import org.apache.parquet.hadoop.metadata.ColumnPath;
import org.apache.parquet.hadoop.metadata.ParquetMetadata;
import org.apache.parquet.hadoop.util.counters.BenchmarkCounter;
import org.apache.parquet.io.ParquetDecodingException;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Map;

public class PegasusParquetChunkReader extends ParquetFileReader {

  private List<ColumnDescriptor> columnDescriptors;
  private final List<BlockMetaData> blocks;

  public PegasusParquetChunkReader(Configuration conf, ParquetMetadata footer, List<ColumnDescriptor> columnDescriptors)
          throws IOException {
    super(conf, null, footer);

    this.columnDescriptors = columnDescriptors;
    this.blocks = footer.getBlocks();

  }

  public PageReadStore getRowGroup(List<FieldVector> fieldVectorList) throws IOException {

    ColumnChunkPageReadStore currentRowGroup = new ColumnChunkPageReadStore(fieldVectorList.size());

    BlockMetaData block = blocks.get(currentBlock);

    List<ColumnChunkMetaData> columnChunkMetaDataList = new ArrayList<ColumnChunkMetaData>();
    for (int i = 0; i < columnDescriptors.size(); ++i) {
      for (ColumnChunkMetaData mc : block.getColumns()) {
        ColumnPath pathKey = mc.getPath();
        if (pathKey.equals(columnDescriptors.get(i).getPath()))
          columnChunkMetaDataList.add(mc);
      }
    }

    if (fieldVectorList.size() != columnDescriptors.size()) {
      throw new ParquetDecodingException("fieldVectorList.size() != columnDescriptors.size()");
    }

    for (int i = 0; i < fieldVectorList.size(); ++i) {
      byte[] data = new byte[(int)fieldVectorList.get(i).getDataBuffer().capacity()];
      fieldVectorList.get(i).getDataBuffer().getBytes(0, data);

      // TODO , use WorkaroundChunk
      final Chunk chunk = new Chunk(new ChunkDescriptor(columnDescriptors.get(i), columnChunkMetaDataList.get(i),0,0), Collections.singletonList(ByteBuffer.wrap(data)));
      currentRowGroup.addColumn(chunk.descriptor.col, chunk.readAllPages());
    }

    if (nextDictionaryReader != null) {
      nextDictionaryReader.setRowGroup(currentRowGroup);
    }

    advanceToNextBlock();

    return currentRowGroup;
  }

}
