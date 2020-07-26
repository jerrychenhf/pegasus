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
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

import org.apache.arrow.memory.BufferAllocator;
import org.apache.arrow.util.AutoCloseables;
import org.apache.arrow.util.Preconditions;
import org.apache.arrow.vector.ipc.message.ArrowDictionaryBatch;
import org.apache.arrow.vector.ipc.message.ArrowRecordBatch;
import org.apache.arrow.vector.ipc.message.MessageMetadataResult;
import org.apache.arrow.vector.ipc.message.MessageSerializer;
import org.apache.arrow.vector.types.pojo.Schema;
import org.apache.pegasus.rpc.grpc.AddWritableBuffer;
import org.apache.pegasus.rpc.grpc.GetReadableBuffer;
import org.apache.pegasus.rpc.impl.Flight.FlightData;
import org.apache.pegasus.rpc.impl.Flight.FlightDescriptor;

import com.google.common.collect.ImmutableList;
import com.google.common.collect.Iterables;
import com.google.common.io.ByteStreams;
import com.google.protobuf.ByteString;
import com.google.protobuf.CodedInputStream;
import com.google.protobuf.CodedOutputStream;
import com.google.protobuf.WireFormat;

import io.grpc.Drainable;
import io.grpc.MethodDescriptor;
import io.grpc.MethodDescriptor.Marshaller;
import io.grpc.protobuf.ProtoUtils;
import io.netty.buffer.ArrowBuf;
import io.netty.buffer.ByteBuf;
import io.netty.buffer.ByteBufInputStream;
import io.netty.buffer.CompositeByteBuf;
import io.netty.buffer.Unpooled;

/**
 * The in-memory representation of FlightData used to manage a stream of Arrow messages.
 */
class FileBatchMessage extends ArrowMessage {
  private MessageMetadataResult messageMetadata;
  
  protected FileBatchMessage(FlightDescriptor descriptor, MessageMetadataResult messageMetadata, ArrowBuf appMetadata,
                       ArrowBuf buf) {
    super(descriptor, messageMetadata, appMetadata, buf);
    this.messageMetadata = messageMetadata;
  }
  
  /** Get the application-specific metadata in this message. The ArrowMessage retains ownership of the buffer. */
  @Override
  public ArrowBuf getApplicationMetadata() {
    return appMetadata;
  }

  @Override
  public MessageMetadataResult asSchemaMessage() {
    throw new RuntimeException("Schema is not supported.");
  }
  
  @Override
  public FlightDescriptor getDescriptor() {
    return descriptor;
  }
  
  @Override
  public HeaderType getMessageType() {
    return HeaderType.getHeader(messageMetadata.headerType());
  }

  @Override
  public Schema asSchema() {
    throw new RuntimeException("Schema is not supported.");
  }
  
  @Override
  public ArrowRecordBatch asRecordBatch() throws IOException {
    throw new RuntimeException("RecordBatch is not supported.");
  }
  
  public FileBatch asFileBatch() throws IOException {
    Preconditions.checkArgument(bufs.size() == 1, "A batch can only be consumed if it contains a single ArrowBuf.");
    Preconditions.checkArgument(getMessageType() == HeaderType.RECORD_BATCH);

    ArrowBuf underlying = bufs.get(0);

    underlying.getReferenceManager().retain();
    return FileBatch.deserializeFileBatch(message, underlying);
  }

  private static ArrowMessage frame(BufferAllocator allocator, final InputStream stream) {

    try {
      FlightDescriptor descriptor = null;
      MessageMetadataResult messageMetadata = null;
      ArrowBuf body = null;
      ArrowBuf appMetadata = null;
      while (stream.available() > 0) {
        int tag = readRawVarint32(stream);
        switch (tag) {

          case DESCRIPTOR_TAG: {
            int size = readRawVarint32(stream);
            byte[] bytes = new byte[size];
            ByteStreams.readFully(stream, bytes);
            descriptor = FlightDescriptor.parseFrom(bytes);
            break;
          }
          case HEADER_TAG: {
            int size = readRawVarint32(stream);
            byte[] bytes = new byte[size];
            ByteStreams.readFully(stream, bytes);
            messageMetadata = MessageMetadataResult.create(ByteBuffer.wrap(bytes), size);
            break;
          }
          case APP_METADATA_TAG: {
            int size = readRawVarint32(stream);
            appMetadata = allocator.buffer(size);
            GetReadableBuffer.readIntoBuffer(stream, appMetadata, size, FAST_PATH);
            break;
          }
          case BODY_TAG:
            if (body != null) {
              // only read last body.
              body.getReferenceManager().release();
              body = null;
            }
            int size = readRawVarint32(stream);
            body = allocator.buffer(size);
            GetReadableBuffer.readIntoBuffer(stream, body, size, FAST_PATH);
            break;

          default:
            // ignore unknown fields.
        }
      }

      return new FileBatchMessage(descriptor, messageMetadata, appMetadata, body);
    } catch (Exception ioe) {
      throw new RuntimeException(ioe);
    }

  }

  protected static class MessageHolderMarshaller implements MethodDescriptor.Marshaller<ArrowMessage> {

    private final BufferAllocator allocator;

    public MessageHolderMarshaller(BufferAllocator allocator) {
      this.allocator = allocator;
    }

    @Override
    public InputStream stream(ArrowMessage value) {
      throw new RuntimeException("Stream is not supported for this Marshaller.");
    }

    @Override
    public ArrowMessage parse(InputStream stream) {
      return FileBatchMessage.frame(allocator, stream);
    }

  }
}
