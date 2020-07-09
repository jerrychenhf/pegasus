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
 * A POJO representation of a LocalReleaseResult associated with a release request of a set of columns for local shared memory read
 */
public class LocalReleaseResult {
  private final int resultCode;

  /**
   * Constructs a new instance.
   *
   * @param resultCode The code of the release operation
   */
  public LocalReleaseResult(int resultCode) {
    super();
    this.resultCode = resultCode;
  }

  /**
   * Constructs from the protocol buffer representation.
   */
  LocalReleaseResult(Flight.LocalReleaseResult pbLocalReleaseResult) throws URISyntaxException {
    resultCode = pbLocalReleaseResult.getResultCode();
  }

  public int getResultCode() {
    return resultCode;
  }

  /**
   * Converts to the protocol buffer representation.
   */
  Flight.LocalReleaseResult toProtocol() {
    return Flight.LocalReleaseResult.newBuilder()
        .setResultCode(LocalReleaseResult.this.resultCode)
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
   * @param serialized The serialized form of the LocalReleaseResult, as returned by {@link #serialize()}.
   * @return The deserialized LocalReleaseResult.
   * @throws IOException if the serialized form is invalid.
   * @throws URISyntaxException if the serialized form contains an unsupported URI format.
   */
  public static LocalReleaseResult deserialize(ByteBuffer serialized) throws IOException, URISyntaxException {
    return new LocalReleaseResult(Flight.LocalReleaseResult.parseFrom(serialized));
  }

  @Override
  public boolean equals(Object o) {
    if (this == o) {
      return true;
    }
    if (o == null || getClass() != o.getClass()) {
      return false;
    }
    LocalReleaseResult that = (LocalReleaseResult) o;
    return resultCode == that.resultCode;
  }

  @Override
  public int hashCode() {
    return Objects.hash(resultCode);
  }

  @Override
  public String toString() {
    return "LocalReleaseResult{" +
        "resultCode=" + resultCode +
        '}';
  }
}
