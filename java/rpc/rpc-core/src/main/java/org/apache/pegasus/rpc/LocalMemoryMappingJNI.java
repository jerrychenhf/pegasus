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

import java.nio.ByteBuffer;


/**
 * A JNI class to call into native for getting the memory mapping buffers
 */
public class LocalMemoryMappingJNI {
  public static native long open(String storeSocketName, int[] mmapFds);
  public static native void close(long conn);

  public static native ByteBuffer getMappedBuffer(long conn, int mmapFd, long mmapSize, long dataOffset, long dataSize);
  public static native ByteBuffer[] getMappedBuffers(long conn, int[] mmapFds, long[] mmapSizes, long[] dataOffsets, long[] dataSizes);
}
