// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#include "org_apache_pegasus_rpc_LocalMemoryMappingJNI.h"

#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "IpcClient.h"

using namespace pegasus;

inline void throw_exception(JNIEnv* env, const char *message ) {
  jclass eclass = env->FindClass("java/io/IOExcepiton");
  env->ThrowNew(eclass,  message);
}

JNIEXPORT jlong JNICALL Java_org_apache_pegasus_rpc_LocalMemoryMappingJNI_open
  (JNIEnv* env, jclass cls, jstring ipcSocketName, jintArray mmapFds) {
  const char* s_name = env->GetStringUTFChars(ipcSocketName, nullptr);
  pegasus::IpcClient* client = new pegasus::IpcClient();
  if(!client->Connect(s_name)) {
    delete client;
    env->ReleaseStringUTFChars(ipcSocketName, s_name);
    throw_exception(env, "Failed to connect to the ipc socket. ");
  }
  
  jsize count = env->GetArrayLength(mmapFds);
  jint* fds = env->GetIntArrayElements(mmapFds, 0);
  
  if(!client->MapFileDescriptors(fds, count)) {
    delete client;
    env->ReleaseIntArrayElements(mmapFds, fds, JNI_ABORT);
    env->ReleaseStringUTFChars(ipcSocketName, s_name);
    throw_exception(env, "Failed to map the file descriptors");
  }

  env->ReleaseIntArrayElements(mmapFds, fds, JNI_ABORT);
  env->ReleaseStringUTFChars(ipcSocketName, s_name);
  return reinterpret_cast<int64_t>(client);
}

JNIEXPORT void JNICALL Java_org_apache_pegasus_rpc_LocalMemoryMappingJNI_close
  (JNIEnv* env, jclass cls, jlong conn) {
  pegasus::IpcClient* client = reinterpret_cast<pegasus::IpcClient*>(conn);
  if(!client->Disconnect()) {
    throw_exception(env, "Failed to disconnect with the Ipc socket.");
  }

  delete client;
}

JNIEXPORT jobject JNICALL Java_org_apache_pegasus_rpc_LocalMemoryMappingJNI_getMappedBuffer(
    JNIEnv* env, jclass cls, jlong conn, jint mmapFd, jlong mmapSize, jlong dataOffset, jlong dataSize) {
  pegasus::IpcClient* client = reinterpret_cast<pegasus::IpcClient*>(conn);
  uint8_t* mapped_pointer = client->LookupOrMmap(mmapFd, mmapSize);
  if (mapped_pointer == nullptr) {
    throw_exception(env, "Failed to mmap for the file descriptor and size. ");
    return nullptr;
  }
  uint8_t* data = mapped_pointer + dataOffset;
  return env->NewDirectByteBuffer(data, dataSize);
}

JNIEXPORT jobjectArray JNICALL Java_org_apache_pegasus_rpc_LocalMemoryMappingJNI_getMappedBuffers(
    JNIEnv* env, jclass cls, jlong conn, jintArray mmapFds, jlongArray mmapSizes, jlongArray dataOffsets, jlongArray dataSizes) {
  pegasus::IpcClient* client = reinterpret_cast<pegasus::IpcClient*>(conn);
  jsize columns = env->GetArrayLength(mmapFds);

  if (columns != env->GetArrayLength(mmapSizes) ||
       columns != env->GetArrayLength(dataOffsets) ||
       columns != env->GetArrayLength(dataSizes)) {
    throw_exception(env, "Mismatch in array length of mmapFds, mmapSizes, dataOffsets, or dataSizes");
    return nullptr;
  }

  jint* in_mmapFds = env->GetIntArrayElements(mmapFds, 0);
  jlong* in_mmapSizes = env->GetLongArrayElements(mmapSizes, 0);
  jlong* in_dataOffsets = env->GetLongArrayElements(dataOffsets, 0);
  jlong* in_dataSizes = env->GetLongArrayElements(dataSizes, 0);
  
  jclass clsByteBuffer = env->FindClass("java/nio/ByteBuffer");

  jobjectArray ret = env->NewObjectArray(columns, clsByteBuffer, nullptr);
  jobject dataBuf;
  for (int i = 0; i < columns; ++i) {
    uint8_t* mapped_pointer = client->LookupOrMmap(in_mmapFds[i], in_mmapSizes[i]);
    if (mapped_pointer == nullptr) {
      // error, release and throw exception
      env->ReleaseIntArrayElements(mmapFds, in_mmapFds, JNI_ABORT);
      env->ReleaseLongArrayElements(mmapSizes, in_mmapSizes, JNI_ABORT);
      env->ReleaseLongArrayElements(dataOffsets, in_dataOffsets, JNI_ABORT);
      env->ReleaseLongArrayElements(dataSizes, in_dataSizes, JNI_ABORT);
      throw_exception(env, "Failed to mmap for the file descriptor and size. ");
      return nullptr;
    }

    uint8_t* data = mapped_pointer + in_dataOffsets[i];
    dataBuf = env->NewDirectByteBuffer(data, in_dataSizes[i]);
    env->SetObjectArrayElement(ret, i, dataBuf);
  }
  
  env->ReleaseIntArrayElements(mmapFds, in_mmapFds, JNI_ABORT);
  env->ReleaseLongArrayElements(mmapSizes, in_mmapSizes, JNI_ABORT);
  env->ReleaseLongArrayElements(dataOffsets, in_dataOffsets, JNI_ABORT);
  env->ReleaseLongArrayElements(dataSizes, in_dataSizes, JNI_ABORT);
  return ret;
}

