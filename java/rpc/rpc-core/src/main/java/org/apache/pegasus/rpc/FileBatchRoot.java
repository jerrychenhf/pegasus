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

import io.netty.buffer.ArrowBuf;
import org.apache.arrow.memory.BufferAllocator;
import org.apache.arrow.vector.FieldVector;

import java.nio.ByteBuffer;
import java.util.List;
import java.util.stream.Collectors;

/**
 * The File Batch which contains chunks of data for columns
 */
public class FileBatchRoot {
  private BufferAllocator allocator;
  private FileBatch fileBatch;
  
  public FileBatchRoot(BufferAllocator allocator) {
    this.allocator = allocator;
  }

  public static FileBatchRoot create(BufferAllocator allocator) {
    return new FileBatchRoot(allocator);
  }
  
  public void load(FileBatch fileBatch) {
    this.fileBatch = fileBatch;
  }
  
  public void clear() {
    fileBatch = null;
  }

  public List<ByteBuffer> getbyteBuffers() {
    return fileBatch.getbyteBuffers();
  }

  public int getRowCount() {
    return fileBatch.getRowCount();
  }

}
