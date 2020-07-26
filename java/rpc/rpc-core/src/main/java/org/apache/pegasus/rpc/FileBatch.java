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
import java.util.ArrayList;
import java.util.List;

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
  private final List<ArrowBuf> buffers;
  
  public FileBatch(List<ArrowBuf> buffers) {
    super();
    this.buffers = buffers;
  }
  
  public static FileBatch deserializeFileBatch(MessageMetadataResult serializedMessage,
                                               ArrowBuf body) throws IOException {
    Message recordBatchMessage = serializedMessage.getMessage();
    RecordBatch recordBatchFB = (RecordBatch) recordBatchMessage.header(new RecordBatch());
    // Now read the body
    int nodesLength = recordBatchFB.nodesLength();
    List<ArrowFieldNode> nodes = new ArrayList<>();
    for (int i = 0; i < nodesLength; ++i) {
      FieldNode node = recordBatchFB.nodes(i);
      if ((int) node.length() != node.length() ||
              (int) node.nullCount() != node.nullCount()) {
        throw new IOException("Cannot currently deserialize record batches with " +
                "node length larger than INT_MAX records.");
      }
      nodes.add(new ArrowFieldNode(node.length(), node.nullCount()));
    }
    List<ArrowBuf> buffers = new ArrayList<>();
    for (int i = 0; i < recordBatchFB.buffersLength(); ++i) {
      Buffer bufferFB = recordBatchFB.buffers(i);
      ArrowBuf vectorBuffer = body.slice(bufferFB.offset(), bufferFB.length());
      buffers.add(vectorBuffer);
    }
    if ((int) recordBatchFB.length() != recordBatchFB.length()) {
      throw new IOException("Cannot currently deserialize record batches with more than INT_MAX records.");
    }
    body.getReferenceManager().release();
    return new FileBatch(buffers);    
  }
  
  /**
   * Releases the buffers.
   */
  @Override
  public void close() {
    if (!closed) {
      closed = true;
      for (ArrowBuf arrowBuf : buffers) {
        arrowBuf.getReferenceManager().release();
      }
    }
  }
}
