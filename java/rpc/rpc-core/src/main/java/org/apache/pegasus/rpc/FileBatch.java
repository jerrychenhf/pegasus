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
import java.nio.ByteOrder;
import java.util.ArrayList;
import java.util.List;
import java.util.stream.Collectors;

import io.netty.buffer.ArrowBuf;
import org.apache.arrow.flatbuf.Buffer;
import org.apache.arrow.flatbuf.FieldNode;
import org.apache.arrow.flatbuf.Message;
import org.apache.arrow.flatbuf.RecordBatch;
import org.apache.arrow.vector.ipc.message.ArrowFieldNode;
import org.apache.arrow.vector.ipc.message.MessageMetadataResult;

/**
 * The File Batch which contains chunks of data for columns
 */
public class FileBatch implements AutoCloseable {
  private boolean closed = false;
//  private final List<ArrowBuf> buffers;
  private final List<ByteBuffer> byteBuffers;
  private final int rowCount;

  public FileBatch(List<ByteBuffer> byteBuffers, int rowCount) {
    super();
    this.byteBuffers = byteBuffers;
    this.rowCount = rowCount;
  }

  public static FileBatch deserializeFileBatch(FileBatchMessageMetadata messageMetaData,
                                               ArrowBuf body) throws IOException {
    //TO DO
    //get the buffers and create the FileBatch
    List<ArrowBuf> buffers = new ArrayList<>();
    List<ByteBuffer> byteBuffers = new ArrayList<>();
    ByteBuffer metaBuffer = messageMetaData.getMessageBuffer();

    //TO DO: make 16 const
    int columns = messageMetaData.getMessageLength() / 16;
    byte[] rowCountBytes = new byte[8];
    metaBuffer.get(rowCountBytes);
    ByteBuffer rowCountBuffer = ByteBuffer.wrap(rowCountBytes);
    int rowCount = (int)rowCountBuffer.order(ByteOrder.LITTLE_ENDIAN).getLong();
    for (int i = 0; i < columns; ++i) {
      // TO DO: get the offset and length of each buffer
      byte[] offsetBytes = new byte[8];
      metaBuffer.get(offsetBytes);
      ByteBuffer offsetBuffer = ByteBuffer.wrap(offsetBytes);
      long offset = offsetBuffer.order(ByteOrder.LITTLE_ENDIAN).getLong();
      byte[] lenBytes = new byte[8];
      metaBuffer.get(lenBytes);
      ByteBuffer lenBuffer = ByteBuffer.wrap(lenBytes);
      int len = (int)lenBuffer.order(ByteOrder.LITTLE_ENDIAN).getLong();


      ByteBuffer byteBuffer = ByteBuffer.allocate(len);
      body.getBytes(offset, byteBuffer);
      byteBuffers.add((ByteBuffer)byteBuffer.clear());
    }

    body.getReferenceManager().release();
    return new FileBatch(byteBuffers, rowCount);
  }

  public List<ByteBuffer> getbyteBuffers() {
    return byteBuffers;
  }

  public int getRowCount() {
    return rowCount;
  }

  /**
   * Releases the buffers.
   */
  @Override
  public void close() {
    if (!closed) {
      closed = true;
    }
  }
}
