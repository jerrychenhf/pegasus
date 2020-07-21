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
 * A POJO representation of a LocalPartitionInfo, metadata associated with a set of data columns of a partition for shared memroy based local read.
 */
public class LocalPartitionInfo {
  private List<LocalColumnInfo> columns;

  /**
   * Constructs a new instance.
   *
   * @param columns A list of LocalColumnInfo for the partition information.
   */
  public LocalPartitionInfo(List<LocalColumnInfo> columns) {
    super();
    Objects.requireNonNull(columns);
    this.columns = columns;
  }

  /**
   * Constructs from the protocol buffer representation.
   */
  LocalPartitionInfo(Flight.LocalPartitionInfo pbLocalPartitionInfo) throws URISyntaxException {
    columns = new ArrayList<>();
    for (final Flight.LocalColumnInfo columnInfo : pbLocalPartitionInfo.getColumnInfoList()) {
      columns.add(new LocalColumnInfo(columnInfo));
    }
  }

  public List<LocalColumnInfo> getColumnInfos() {
    return columns;
  }

  /**
   * Converts to the protocol buffer representation.
   */
  Flight.LocalPartitionInfo toProtocol() {
    return Flight.LocalPartitionInfo.newBuilder()
        .addAllColumnInfo(columns.stream().map(t -> t.toProtocol()).collect(Collectors.toList()))
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
   * @param serialized The serialized form of the LocalPartitionInfo, as returned by {@link #serialize()}.
   * @return The deserialized LocalPartitionInfo.
   * @throws IOException if the serialized form is invalid.
   * @throws URISyntaxException if the serialized form contains an unsupported URI format.
   */
  public static LocalPartitionInfo deserialize(ByteBuffer serialized) throws IOException, URISyntaxException {
    return new LocalPartitionInfo(Flight.LocalPartitionInfo.parseFrom(serialized));
  }

  @Override
  public boolean equals(Object o) {
    if (this == o) {
      return true;
    }
    if (o == null || getClass() != o.getClass()) {
      return false;
    }
    LocalPartitionInfo that = (LocalPartitionInfo) o;
    return columns.equals(that.columns);
  }

  @Override
  public int hashCode() {
    return Objects.hash(columns);
  }

  @Override
  public String toString() {
    return "LocalPartitionInfo{" +
        "columns=" + columns +
        '}';
  }
}
