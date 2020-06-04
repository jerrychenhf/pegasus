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

import java.util.ArrayList;
import java.util.List;

import io.netty.buffer.ArrowBuf;

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
  
  public static FileBatch deserializeFileBatch(FileBatchMessageMetadata messageMetaData,
                                               ArrowBuf body) {
    //TO DO
    //get the buffers and create the FileBatch
    List<ArrowBuf> buffers = new ArrayList<>();
    
    //TO DO: get columns
    int columns = 0;
    for (int i = 0; i < columns; ++i) {
      // TO DO: get the offset and length of each buffer
      int offset = 0;
      int len = 0;
      ArrowBuf vectorBuffer = body.slice(offset, len);
      buffers.add(vectorBuffer);
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
