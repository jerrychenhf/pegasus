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

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.net.URISyntaxException;
import java.nio.ByteBuffer;
import java.nio.channels.Channels;
import java.util.ArrayList;
import java.util.List;
import java.util.Objects;
import java.util.stream.Collectors;

import org.apache.pegasus.rpc.impl.Flight;

/**
 * A POJO representation of a LocalColumnChunkInfo, metadata associated with one column of a partition for shared memroy based local read.
 */
public class LocalColumnChunkInfo {
  private final int chunkIndex;
  private final int dataOffset;
  private final long dataSize;
  private final int mmapFd;
  private final long mmapSize;
  private final int rowCounts;

  /**
   * Constructs a new instance.
   *
   * @param chunkIndex The column index of the column
   * @param dataOffset The data offset in the mmap of this column
   * @param dataSize The number of bytes in the column
   * @param mmapFd The mmap file descriptor
   * @param mmapSize The number of bytes for the mmap.
   */
  public LocalColumnChunkInfo(int chunkIndex, int dataOffset, long dataSize,
      int mmapFd, long mmapSize, int rowCounts) {
    super();
    this.chunkIndex = chunkIndex;
    this.dataOffset = dataOffset;
    this.dataSize = dataSize;
    this.mmapFd = mmapFd;
    this.mmapSize = mmapSize;
    this.rowCounts = rowCounts;
  }

  /**
   * Constructs from the protocol buffer representation.
   */
  LocalColumnChunkInfo(Flight.LocalColumnChunkInfo pbLocalColumnChunkInfo) throws URISyntaxException {
    chunkIndex = pbLocalColumnChunkInfo.getChunkIndex();
    dataOffset = pbLocalColumnChunkInfo.getDataOffset();
    dataSize = pbLocalColumnChunkInfo.getDataSize();
    mmapFd = pbLocalColumnChunkInfo.getMmapFd();
    mmapSize = pbLocalColumnChunkInfo.getMmapSize();
    rowCounts = (int)pbLocalColumnChunkInfo.getRowCounts();
  }

  public int getChunkIndex() {
    return chunkIndex;
  }

  public int getDataOffset() {
    return dataOffset;
  }
  
  public long getDataSize() {
    return dataSize;
  }
  
  public int getMmapFd() {
    return mmapFd;
  }

  public long getMmapSize() {
    return mmapSize;
  }

  public int getRowCounts() {
        return rowCounts;
    }

  /**
   * Converts to the protocol buffer representation.
   */
  Flight.LocalColumnChunkInfo toProtocol() {
    return Flight.LocalColumnChunkInfo.newBuilder()
        .setChunkIndex(LocalColumnChunkInfo.this.chunkIndex)
        .setDataOffset(LocalColumnChunkInfo.this.dataOffset)
        .setDataSize(LocalColumnChunkInfo.this.dataSize)
        .setMmapFd(LocalColumnChunkInfo.this.mmapFd)
        .setMmapSize(LocalColumnChunkInfo.this.mmapSize)
        .setRowCounts(LocalColumnChunkInfo.this.rowCounts)
        .build();
  }

  /**
   * Get the serialized form of this protocol message.
   *
   * <p>Intended to help interoperability by allowing non-Flight services to still return Flight types.
   */
  public ByteBuffer serialize() {
    return ByteBuffer.wrap(toProtocol().toByteArray());
  }

  /**
   * Parse the serialized form of this protocol message.
   *
   * <p>Intended to help interoperability by allowing Flight clients to obtain stream info from non-Flight services.
   *
   * @param serialized The serialized form of the LocalColumnChunkInfo, as returned by {@link #serialize()}.
   * @return The deserialized LocalColumnChunkInfo.
   * @throws IOException if the serialized form is invalid.
   * @throws URISyntaxException if the serialized form contains an unsupported URI format.
   */
  public static LocalColumnChunkInfo deserialize(ByteBuffer serialized) throws IOException, URISyntaxException {
    return new LocalColumnChunkInfo(Flight.LocalColumnChunkInfo.parseFrom(serialized));
  }

  @Override
  public boolean equals(Object o) {
    if (this == o) {
      return true;
    }
    if (o == null || getClass() != o.getClass()) {
      return false;
    }
    LocalColumnChunkInfo that = (LocalColumnChunkInfo) o;
    return chunkIndex == that.chunkIndex &&
        dataOffset == that.dataOffset &&
        dataSize == that.dataSize &&
        mmapFd == that.mmapFd &&
        mmapSize == that.mmapSize &&
        rowCounts ==  that.rowCounts;
  }

  @Override
  public int hashCode() {
    return Objects.hash(chunkIndex, dataOffset, dataSize, mmapFd, mmapSize, rowCounts);
  }

  @Override
  public String toString() {
    return "LocalColumnChunkInfo{" +
        "chunkIndex=" + chunkIndex +
        ", dataOffset=" + dataOffset +
        ", dataSize=" + dataSize +
        ", mmapFd=" + mmapFd +
        ", mmapSize=" + mmapSize +
        ", rowCounts=" + rowCounts +
        '}';
  }
}
