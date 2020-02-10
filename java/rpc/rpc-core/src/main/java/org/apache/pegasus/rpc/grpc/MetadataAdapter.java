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

package org.apache.pegasus.rpc.grpc;

import java.util.Set;
import java.util.stream.Collectors;

import org.apache.pegasus.rpc.CallHeaders;

import io.grpc.Metadata;
import io.grpc.Metadata.Key;

/**
 * A mutable adapter between the gRPC Metadata object and the Flight headers interface.
 *
 * <p>This allows us to present the headers (metadata) from gRPC without copying to/from our own object.
 */
class MetadataAdapter implements CallHeaders {

  private final Metadata metadata;

  MetadataAdapter(Metadata metadata) {
    this.metadata = metadata;
  }

  @Override
  public String get(String key) {
    return this.metadata.get(Key.of(key, Metadata.ASCII_STRING_MARSHALLER));
  }

  @Override
  public Iterable<String> getAll(String key) {
    return this.metadata.getAll(Key.of(key, Metadata.ASCII_STRING_MARSHALLER));
  }

  @Override
  public void insert(String key, String value) {
    this.metadata.put(Key.of(key, Metadata.ASCII_STRING_MARSHALLER), value);
  }

  @Override
  public Set<String> keys() {
    // Remove binary keys - we don't expose those
    return this.metadata.keys().stream().filter(key -> !key.endsWith(Metadata.BINARY_HEADER_SUFFIX))
        .collect(Collectors.toSet());
  }

  @Override
  public boolean containsKey(String key) {
    final Key<?> grpcKey = Key.of(key, Metadata.ASCII_STRING_MARSHALLER);
    return this.metadata.containsKey(grpcKey);
  }

  public String toString() {
    return this.metadata.toString();
  }
}
