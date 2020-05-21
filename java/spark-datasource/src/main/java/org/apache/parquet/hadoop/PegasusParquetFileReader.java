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

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Map;

import com.google.common.collect.Maps;
import org.apache.hadoop.conf.Configuration;
import org.apache.parquet.column.ColumnDescriptor;
import org.apache.parquet.column.page.PageReadStore;
import org.apache.parquet.hadoop.ParquetFileReader;
import org.apache.parquet.hadoop.metadata.ColumnChunkMetaData;
import org.apache.parquet.hadoop.metadata.ColumnPath;
import org.apache.parquet.hadoop.metadata.BlockMetaData;
import org.apache.parquet.hadoop.metadata.ParquetMetadata;
import org.apache.parquet.hadoop.util.counters.BenchmarkCounter;

public class PegasusParquetFileReader extends ParquetFileReader {

  private Map<ColumnPath, Integer> pathToColumnIndexMap = Collections.emptyMap();

  public PegasusParquetFileReader(Configuration conf, ParquetMetadata footer)
          throws IOException {
    super(conf, null, footer);

    List<String[]> paths = footer.getFileMetaData().getSchema().getPaths();
    int size = paths.size();
    // TODO only required columns needed in pathToColumnIndexMap, but include all columns for now,
    // better to do the filter.
    pathToColumnIndexMap = Maps.newHashMapWithExpectedSize(size);
    for (int i = 0; i < size; i++) {
      pathToColumnIndexMap.put(ColumnPath.get(paths.get(i)), i);
    }
  }

  public PageReadStore readNextRowGroup() throws IOException {
    // if binaryCache enable or mixed read enable, we should run this logic.
    if (currentBlock == blocks.size()) {
      return null;
    }

    BlockMetaData block = blocks.get(currentBlock);
    if (block.getRowCount() == 0) {
      throw new RuntimeException("Illegal row group of 0 rows");
    }
    this.currentRowGroup = new ColumnChunkPageReadStore(block.getRowCount());

    List<ColumnChunk> allChunks = new ArrayList<>();
    for (ColumnChunkMetaData mc : block.getColumns()) {
      ColumnPath pathKey = mc.getPath();
      BenchmarkCounter.incrementTotalBytes(mc.getTotalSize());
      ColumnDescriptor columnDescriptor = paths.get(pathKey);
      if (columnDescriptor != null) {
        long startingPos = mc.getStartingPos();
        int totalSize = (int) mc.getTotalSize();
        int columnIndex = pathToColumnIndexMap.get(pathKey);
        ColumnChunk columnChunk =
          new ColumnChunk(columnDescriptor, mc, startingPos, totalSize, columnIndex);
        allChunks.add(columnChunk);
      }
    }
    // actually read all the chunks
    for (ColumnChunk columnChunk : allChunks) {
      final Chunk chunk = columnChunk.read();
      currentRowGroup.addColumn(chunk.descriptor.col, chunk.readAllPages());
    }

    // avoid re-reading bytes the dictionary reader is used after this call
    if (nextDictionaryReader != null) {
      nextDictionaryReader.setRowGroup(currentRowGroup);
    }

    advanceToNextBlock();

    return currentRowGroup;
  }

  public class ColumnChunk {
    private final long offset;
    private final int length;
    private final ChunkDescriptor descriptor;
    private final int columnIndex;

    ColumnChunk(
        ColumnDescriptor col,
        ColumnChunkMetaData metadata,
        long offset,
        int length,
        int columnIndex) {
      this.offset = offset;
      this.length = length;
      this.columnIndex = columnIndex;
      descriptor = new ChunkDescriptor(col, metadata, offset, length);
    }

    Chunk read() throws IOException {
      // TODO impl mixed read model.
      byte[] chunksBytes = getColumnChunkBytes();
      return new WorkaroundChunk(
          descriptor, Collections.singletonList(ByteBuffer.wrap(chunksBytes)), f);
    }

    private byte[] getColumnChunkBytes() {
    	//TO DO: get the correponsing column chunk bytes
    	byte[] data = new byte[length];
    	return data;
    }
  }
}
