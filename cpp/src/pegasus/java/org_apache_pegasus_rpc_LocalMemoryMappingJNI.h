/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class org_apache_pegasus_rpc_LocalMemoryMappingJNI */

#ifndef _Included_org_apache_pegasus_rpc_LocalMemoryMappingJNI
#define _Included_org_apache_pegasus_rpc_LocalMemoryMappingJNI
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     org_apache_pegasus_rpc_LocalMemoryMappingJNI
 * Method:    open
 * Signature: (Ljava/lang/String;[I)J
 */
JNIEXPORT jlong JNICALL Java_org_apache_pegasus_rpc_LocalMemoryMappingJNI_open
  (JNIEnv *, jclass, jstring, jintArray);

/*
 * Class:     org_apache_pegasus_rpc_LocalMemoryMappingJNI
 * Method:    close
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_org_apache_pegasus_rpc_LocalMemoryMappingJNI_close
  (JNIEnv *, jclass, jlong);

/*
 * Class:     org_apache_pegasus_rpc_LocalMemoryMappingJNI
 * Method:    getMappedBuffer
 * Signature: (JIJJJ)Ljava/nio/ByteBuffer;
 */
JNIEXPORT jobject JNICALL Java_org_apache_pegasus_rpc_LocalMemoryMappingJNI_getMappedBuffer
  (JNIEnv *, jclass, jlong, jint, jlong, jlong, jlong);

/*
 * Class:     org_apache_pegasus_rpc_LocalMemoryMappingJNI
 * Method:    getMappedBuffers
 * Signature: (J[I[J[J[J)[Ljava/nio/ByteBuffer;
 */
JNIEXPORT jobjectArray JNICALL Java_org_apache_pegasus_rpc_LocalMemoryMappingJNI_getMappedBuffers
  (JNIEnv *, jclass, jlong, jintArray, jlongArray, jlongArray, jlongArray);

#ifdef __cplusplus
}
#endif
#endif
