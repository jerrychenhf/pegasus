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

package org.apache.pegasus.rpc;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;



/**
 * A class to help map the memory columns in local partition information and return as ByteBuffer (Direct)
 */
public class LocalPartitionReader {
  private LocalPartitionInfo localPartitionInfo;

  private int columns = 0;
  private int chunkCount = 0;
  private int currentChunk = 0;
  private int[] mmapFds;
  private long[] mmapSizes;
  private long[] dataOffsets;
  private long[] dataSizes;

  /**
   * Constructs a new instance.
   *
   * @param localPartitionInfo The LocalPartitionInfo from which to construct the shared memory buffers for the columns
   */
  public LocalPartitionReader(LocalPartitionInfo localPartitionInfo) {
    this.localPartitionInfo = localPartitionInfo;
    if (localPartitionInfo != null && localPartitionInfo.getColumnInfos().size() != 0) {
      this.columns = localPartitionInfo.getColumnInfos().size();
      LocalColumnInfo column = localPartitionInfo.getColumnInfos().get(0);
      this.chunkCount = column.getColumnChunkInfos().size();
      this.mmapFds = new int[columns];
      this.mmapSizes = new long[columns];
      this.dataOffsets = new long[columns];
      this.dataSizes = new long[columns];
    }
  }

  /**
   * Do the mapping and retunr the the list of the mapped byte buffers
   */
  public List<ByteBuffer> next() throws IOException {
    if(currentChunk >= chunkCount) 
      return null;

    // map the shared memory of each column in batch
    List<ByteBuffer> columnBuffers = mapColumnsBatch(currentChunk);
    currentChunk++;
    return columnBuffers;
  }
  
  private List<ByteBuffer> mapColumns(int chunkIndex) throws IOException   {
    // map the shared memory of each column
    List<LocalColumnInfo> columnInfos = localPartitionInfo.getColumnInfos();
    List<ByteBuffer> columnBuffers = new ArrayList<ByteBuffer>(columnInfos.size());
    for( LocalColumnInfo columnInfo : columnInfos) {
      ByteBuffer columnBuffer = readColumn(columnInfo, chunkIndex);
      columnBuffers.add(columnBuffer);
    }
    return columnBuffers;
  }

  private List<ByteBuffer> mapColumnsBatch(int chunkIndex) throws IOException   {
    // map the shared memory of each column
    List<LocalColumnInfo> columnInfos = localPartitionInfo.getColumnInfos();
    int column = 0;
    for( LocalColumnInfo columnInfo : columnInfos) {
      List<LocalColumnChunkInfo> chunks = columnInfo.getColumnChunkInfos();
      LocalColumnChunkInfo chunk = chunks.get(chunkIndex);
      mmapFds[column] = chunk.getMmapFd();
      mmapSizes[column] = chunk.getMmapSize();
      dataOffsets[column] = chunk.getDataOffset();
      dataSizes[column] = chunk.getDataSize();
      column++;
    }

    ByteBuffer[]  buffers =  LocalMemoryMappingJNI.getMappedBuffers(
      mmapFds, mmapSizes, dataOffsets, dataSizes);
    List<ByteBuffer> columnBuffers = Arrays.asList(buffers);  
    return columnBuffers;
  }

  private ByteBuffer readColumn(LocalColumnInfo columnInfo, int chunkIndex) throws IOException {
    List<LocalColumnChunkInfo> chunks = columnInfo.getColumnChunkInfos();
    LocalColumnChunkInfo chunk = chunks.get(chunkIndex);
    return readColumnChunk(chunk);
  }
  
  private ByteBuffer readColumnChunk(LocalColumnChunkInfo chunk) throws IOException {
    return LocalMemoryMappingJNI.getMappedBuffer(chunk.getMmapFd(), chunk.getMmapSize(),
      chunk.getDataOffset(), chunk.getDataSize());
  }

}
