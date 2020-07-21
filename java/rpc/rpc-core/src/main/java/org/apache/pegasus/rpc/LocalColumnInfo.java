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
 * A POJO representation of a LocalColumnInfo, metadata associated with one column of a partition for shared memroy based local read.
 */
public class LocalColumnInfo {
  private final int columnIndex;
  private List<LocalColumnChunkInfo> chunks;

  /**
   * Constructs a new instance.
   *
   * @param columnIndex The column index of the column
   * @param dataOffset The data offset in the mmap of this column
   * @param dataSize The number of bytes in the column
   * @param mmapFd The mmap file descriptor
   * @param mmapSize The number of bytes for the mmap.
   */
  public LocalColumnInfo(int columnIndex, List<LocalColumnChunkInfo> chunks) {
    super();
    this.columnIndex = columnIndex;
    Objects.requireNonNull(chunks);
    this.chunks = chunks;
  }

  /**
   * Constructs from the protocol buffer representation.
   */
  LocalColumnInfo(Flight.LocalColumnInfo pbLocalColumnInfo) throws URISyntaxException {
    columnIndex = pbLocalColumnInfo.getColumnIndex();
    chunks = new ArrayList<>();
    for (final Flight.LocalColumnChunkInfo columnChunkInfo : pbLocalColumnInfo.getColumnChunkInfoList()) {
      chunks.add(new LocalColumnChunkInfo(columnChunkInfo));
    }
  }

  public int getColumnIndex() {
    return columnIndex;
  }

  public List<LocalColumnChunkInfo> getColumnChunkInfos() {
    return chunks;
  }

  /**
   * Converts to the protocol buffer representation.
   */
  Flight.LocalColumnInfo toProtocol() {
    return Flight.LocalColumnInfo.newBuilder()
        .setColumnIndex(LocalColumnInfo.this.columnIndex)
        .addAllColumnChunkInfo(chunks.stream().map(t -> t.toProtocol()).collect(Collectors.toList()))
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
   * @param serialized The serialized form of the LocalColumnInfo, as returned by {@link #serialize()}.
   * @return The deserialized LocalColumnInfo.
   * @throws IOException if the serialized form is invalid.
   * @throws URISyntaxException if the serialized form contains an unsupported URI format.
   */
  public static LocalColumnInfo deserialize(ByteBuffer serialized) throws IOException, URISyntaxException {
    return new LocalColumnInfo(Flight.LocalColumnInfo.parseFrom(serialized));
  }

  @Override
  public boolean equals(Object o) {
    if (this == o) {
      return true;
    }
    if (o == null || getClass() != o.getClass()) {
      return false;
    }
    LocalColumnInfo that = (LocalColumnInfo) o;
    return columnIndex == that.columnIndex &&
        chunks.equals(that.chunks);
  }

  @Override
  public int hashCode() {
    return Objects.hash(columnIndex, chunks);
  }

  @Override
  public String toString() {
    return "LocalColumnInfo{" +
        "columnIndex=" + columnIndex +
        "columns=" + chunks +
        '}';
  }
}
