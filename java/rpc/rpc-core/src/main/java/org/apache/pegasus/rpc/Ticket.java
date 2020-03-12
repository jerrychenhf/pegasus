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
import java.util.Arrays;
import java.util.stream.Collectors;

import org.apache.pegasus.rpc.impl.Flight;

import com.google.protobuf.ByteString;

/**
 * Endpoint for a particular stream.
 */
public class Ticket implements java.io.Serializable {
  private final byte[] datasetPath;
  private final byte[] partitionIdentity;
  private final byte[] schema;
  private final int[] columnIndices;

  public Ticket(byte[] datasetPath, byte[] partitionIdentity, byte[] schema, int[] columnIndices) {
    super();
    this.datasetPath = datasetPath;
    this.partitionIdentity = partitionIdentity;
    this.schema = schema;
    this.columnIndices = columnIndices;
  }

  public byte[] getDatasetPath() {
    return datasetPath;
  }

  public byte[] getPartitionIdentity() {
    return partitionIdentity;
  }

  public int[] getcolumnIndices() {
    return columnIndices;
  }

  Ticket(org.apache.pegasus.rpc.impl.Flight.Ticket ticket) {
    this.datasetPath = ticket.getDatasetPath().toByteArray();
    this.partitionIdentity = ticket.getPartitionIdentity().toByteArray();
    this.schema = ticket.getSchema().toByteArray();
    this.columnIndices = ticket.getColumnIndiceList().stream().mapToInt(i->i).toArray();
  }

  Flight.Ticket toProtocol() {
    Flight.Ticket.Builder b = Flight.Ticket.newBuilder();

    if(datasetPath != null && datasetPath.length > 0) {
      b.setDatasetPath(ByteString.copyFrom(datasetPath));
    }
    if (partitionIdentity != null && partitionIdentity.length > 0) {
      b.setPartitionIdentity(ByteString.copyFrom(partitionIdentity));
    }
    if (schema != null && schema.length > 0) {
      b.setSchema(ByteString.copyFrom(schema));
    }
    if (columnIndices != null && columnIndices.length > 0 ) {
      b.addAllColumnIndice(Arrays.stream(columnIndices).boxed().collect(Collectors.toList()));
    }

    return b.build();
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
   * @param serialized The serialized form of the Ticket, as returned by {@link #serialize()}.
   * @return The deserialized Ticket.
   * @throws IOException if the serialized form is invalid.
   */
  public static Ticket deserialize(ByteBuffer serialized) throws IOException {
    return new Ticket(Flight.Ticket.parseFrom(serialized));
  }

  @Override
  public int hashCode() {
    final int prime = 31;
    int result = 1;
    result = prime * result
            + ((datasetPath == null) ? 0 : Arrays.hashCode(datasetPath))
            + ((partitionIdentity == null) ? 0 : Arrays.hashCode(partitionIdentity))
            + ((schema == null) ? 0 : Arrays.hashCode(schema))
            + ((columnIndices == null ) ? 0 : columnIndices.hashCode());
    return result;
  }

  @Override
  public boolean equals(Object obj) {
    if (this == obj) {
      return true;
    }
    if (obj == null) {
      return false;
    }
    if (getClass() != obj.getClass()) {
      return false;
    }
    Ticket other = (Ticket) obj;
    if (!Arrays.equals(datasetPath, other.datasetPath)) {
      return false;
    }
    if (!Arrays.equals(partitionIdentity, other.partitionIdentity)) {
      return false;
    }
    if (!Arrays.equals(schema, other.schema)) {
      return false;
    }
    if (columnIndices == null) {
      if (other.columnIndices != null) {
        return false;
      }
    } else if (!Arrays.equals(columnIndices, other.columnIndices)) {
      return false;
    }
    return true;
  }


}
