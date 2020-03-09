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
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import org.apache.arrow.util.Preconditions;
import org.apache.pegasus.rpc.impl.Flight;
import org.apache.pegasus.rpc.impl.Flight.FlightDescriptor.DescriptorType;

import com.google.common.base.Joiner;
import com.google.common.collect.ImmutableList;
import com.google.protobuf.ByteString;

/**
 * An identifier for a particular set of data.  This can either be an opaque command that generates
 * the data or a static "path" to the data.  This is a POJO wrapper around the protobuf message with
 * the same name.
 */
public class FlightDescriptor {

  public static final String COLUMN_NAMES = "column.names";
  public static final String TABLE_LOCATION = "table.location";
  public static final String PROVIDER = "provider";
  public static final String PROVIDER_SPARK = "SPARK";
  public static final String PROVIDER_PEGASUS = "PEGASUS";

  private boolean isCmd;
  private List<String> path;
  private byte[] cmd;
  private Map<String, String> properties = new HashMap<>();

  private FlightDescriptor(boolean isCmd, List<String> path, byte[] cmd,
                           Map<String, String> properties) {
    super();
    this.isCmd = isCmd;
    this.path = path;
    this.cmd = cmd;
    this.properties = properties;
  }

  public static FlightDescriptor command(byte[] cmd) {
    return new FlightDescriptor(true, null, cmd, new HashMap<>());
  }

  public static FlightDescriptor path(Iterable<String> path) {
    return new FlightDescriptor(false, ImmutableList.copyOf(path), null, new HashMap<>());
  }

  public static FlightDescriptor path(String...path) {
    return new FlightDescriptor(false, ImmutableList.copyOf(path), null, new HashMap<>());
  }

  public static FlightDescriptor path(Iterable<String> path, Map<String, String> properties) {
    return new FlightDescriptor(false, ImmutableList.copyOf(path), null, properties);
  }

  FlightDescriptor(Flight.FlightDescriptor descriptor) {
    if (descriptor.getType() == DescriptorType.CMD) {
      isCmd = true;
      cmd = descriptor.getCmd().toByteArray();
    } else if (descriptor.getType() == DescriptorType.PATH) {
      isCmd = false;
      path = descriptor.getPathList();
    } else {
      throw new UnsupportedOperationException();
    }
    properties = descriptor.getPropertiesMap();
  }

  public boolean isCommand() {
    return isCmd;
  }

  public List<String> getPath() {
    Preconditions.checkArgument(!isCmd);
    return path;
  }

  public byte[] getCommand() {
    Preconditions.checkArgument(isCmd);
    return cmd;
  }

  public Map<String, String> getProperties() {
    return properties;
  }

  Flight.FlightDescriptor toProtocol() {
    Flight.FlightDescriptor.Builder b = Flight.FlightDescriptor.newBuilder();
    if (properties != null && properties.size() > 0) {
      b.putAllProperties(properties);
    }
    if (isCmd) {
      return b.setType(DescriptorType.CMD).setCmd(ByteString.copyFrom(cmd)).build();
    }
    return b.setType(DescriptorType.PATH).addAllPath(path).build();
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
   * @param serialized The serialized form of the FlightDescriptor, as returned by {@link #serialize()}.
   * @return The deserialized FlightDescriptor.
   * @throws IOException if the serialized form is invalid.
   */
  public static FlightDescriptor deserialize(ByteBuffer serialized) throws IOException {
    return new FlightDescriptor(Flight.FlightDescriptor.parseFrom(serialized));
  }

  @Override
  public String toString() {
    if (isCmd) {
      return toHex(cmd);
    } else {
      return Joiner.on('.').join(path);
    }
  }

  private String toHex(byte[] bytes) {
    StringBuilder sb = new StringBuilder();
    for (byte b : bytes) {
      sb.append(String.format("%02X ", b));
    }
    return sb.toString();
  }

  @Override
  public int hashCode() {
    final int prime = 31;
    int result = 1;
    result = prime * result + ((cmd == null) ? 0 : Arrays.hashCode(cmd));
    result = prime * result + (isCmd ? 1231 : 1237);
    result = prime * result + ((path == null) ? 0 : path.hashCode());
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
    FlightDescriptor other = (FlightDescriptor) obj;
    if (cmd == null) {
      if (other.cmd != null) {
        return false;
      }
    } else if (!Arrays.equals(cmd, other.cmd)) {
      return false;
    }
    if (isCmd != other.isCmd) {
      return false;
    }
    if (path == null) {
      if (other.path != null) {
        return false;
      }
    } else if (!path.equals(other.path)) {
      return false;
    }
    if(properties == null) {
      if (other.properties != null) {
        return false;
      }
    } else if (!properties.equals(other.properties)) {
      return false;
    }
    return true;
  }


}
