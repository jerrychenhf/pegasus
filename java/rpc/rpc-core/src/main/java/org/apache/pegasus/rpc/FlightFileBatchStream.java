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
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.LinkedBlockingQueue;

import org.apache.arrow.memory.BufferAllocator;
import org.apache.arrow.util.AutoCloseables;
import org.apache.arrow.vector.FieldVector;
import org.apache.arrow.vector.VectorLoader;
import org.apache.arrow.vector.VectorSchemaRoot;
import org.apache.arrow.vector.dictionary.Dictionary;
import org.apache.arrow.vector.dictionary.DictionaryProvider;
import org.apache.arrow.vector.ipc.message.ArrowDictionaryBatch;
import org.apache.arrow.vector.ipc.message.ArrowRecordBatch;
import org.apache.arrow.vector.types.pojo.Field;
import org.apache.arrow.vector.types.pojo.Schema;
import org.apache.arrow.vector.util.DictionaryUtility;
import org.apache.pegasus.rpc.ArrowMessage.HeaderType;
import org.apache.pegasus.rpc.grpc.StatusUtils;

import com.google.common.util.concurrent.SettableFuture;

import io.grpc.stub.StreamObserver;
import io.netty.buffer.ArrowBuf;

/**
 * An adaptor between protobuf streams and flight data streams.
 */
public class FlightFileBatchStream extends FlightStream {
  protected final SettableFuture<FileBatchRoot> fileBatchRoot = SettableFuture.create();
  
  protected volatile FileBatchRoot fulfilledFileBatchRoot;
  
  public FlightFileBatchStream(BufferAllocator allocator, int pendingTarget, Cancellable cancellable, Requestor requestor) {
    super(allocator, pendingTarget, cancellable, requestor);
  }

  /**
   * Closes the stream (freeing any existing resources).
   *
   * <p>If the stream is isn't complete and is cancellable this method will cancel the stream first.</p>
   */
  public void close() throws Exception {
    final List<AutoCloseable> closeables = new ArrayList<>();
    // cancellation can throw, but we still want to clean up resources, so make it an AutoCloseable too
    closeables.add(() -> {
      if (!completed && cancellable != null) {
        cancel("Stream closed before end.", /* no exception to report */ null);
      }
    });
//    closeables.add(root.get());
    closeables.add(applicationMetadata);
    closeables.addAll(queue);
    if (dictionaries != null) {
      dictionaries.getDictionaryIds().forEach(id -> closeables.add(dictionaries.lookup(id).getVector()));
    }

    AutoCloseables.close(closeables);
  }

  @Override
  public Schema getSchema() {
    throw new RuntimeException("Schema is not support!");
  }
  
  /**
   * Get the descriptor for this stream. Only applicable on the server side of a DoPut operation. Will block until the
   * client sends the descriptor.
   */
  public FlightDescriptor getDescriptor() {
    throw new RuntimeException("Descriptor is not support!");
  }
  
  /**
   * Blocking request to load next item into list.
   * @return Whether or not more data was found.
   */
  @Override
  public boolean next() {
    try {
      // make sure we have the file batch
      fileBatchRoot.get().clear();

      if (completed && queue.isEmpty()) {
        return false;
      }


      pending--;
      requestOutstanding();

      Object data = queue.take();
      if (DONE == data) {
        queue.put(DONE);
        completed = true;
        return false;
      } else if (DONE_EX == data) {
        queue.put(DONE_EX);
        if (ex instanceof Exception) {
          throw (Exception) ex;
        } else {
          throw new Exception(ex);
        }
      } else {
        try (FileBatchMessage msg = ((FileBatchMessage) data)) {
          if (msg.getMessageType() == HeaderType.RECORD_BATCH) {
            try (FileBatch fb = msg.asFileBatch()) {
              fulfilledFileBatchRoot.load(fb);
            }
            if (this.applicationMetadata != null) {
              this.applicationMetadata.close();
            }
            this.applicationMetadata = msg.getApplicationMetadata();
            if (this.applicationMetadata != null) {
              this.applicationMetadata.getReferenceManager().retain();
            }
          } else {
            throw new UnsupportedOperationException("Message type is unsupported: " + msg.getMessageType());
          }
          return true;
        }
      }
    } catch (RuntimeException e) {
      throw e;
    } catch (ExecutionException e) {
      throw StatusUtils.fromThrowable(e.getCause());
    } catch (Exception e) {
      throw new RuntimeException(e);
    }
  }

  public FileBatchRoot getFileBatchRoot() {
    try {
      return fileBatchRoot.get();
    } catch (InterruptedException e) {
      throw CallStatus.INTERNAL.withCause(e).toRuntimeException();
    } catch (ExecutionException e) {
      throw StatusUtils.fromThrowable(e.getCause());
    }
  }

  private class Observer implements StreamObserver<ArrowMessage> {

    Observer() {
      super();
    }

    @Override
    public void onNext(ArrowMessage msg) {
      requestOutstanding();
      switch (msg.getMessageType()) {
        case SCHEMA: {
          Schema schema = msg.asSchema();
          final List<Field> fields = new ArrayList<>();
          final Map<Long, Dictionary> dictionaryMap = new HashMap<>();
          for (final Field originalField : schema.getFields()) {
            final Field updatedField = DictionaryUtility.toMemoryFormat(originalField, allocator, dictionaryMap);
            fields.add(updatedField);
          }
          for (final Map.Entry<Long, Dictionary> entry : dictionaryMap.entrySet()) {
            dictionaries.put(entry.getValue());
          }
          schema = new Schema(fields, schema.getCustomMetadata());
          fulfilledFileBatchRoot = FileBatchRoot.create(allocator);
          descriptor = msg.getDescriptor() != null ? new FlightDescriptor(msg.getDescriptor()) : null;
          fileBatchRoot.set(fulfilledFileBatchRoot);

          break;
        }
        case RECORD_BATCH:
          queue.add(msg);
          break;
        case DICTIONARY_BATCH:
        case NONE:
        case TENSOR:
        default:
          queue.add(DONE_EX);
          ex = new UnsupportedOperationException("Unable to handle message of type: " + msg.getMessageType());

      }

    }

    @Override
    public void onError(Throwable t) {
      ex = t;
      queue.add(DONE_EX);
      fileBatchRoot.setException(t);
    }

    @Override
    public void onCompleted() {
      // Depends on gRPC calling onNext and onCompleted non-concurrently
      if (!fileBatchRoot.isDone()) {
        fileBatchRoot.setException(
            CallStatus.INTERNAL.withDescription("Stream completed without receiving schema.").toRuntimeException());
      }
      queue.add(DONE);
    }
  }

  /**
   * Cancels sending the stream to a client.
   *
   * @throws UnsupportedOperationException on a stream being uploaded from the client.
   */
  public void cancel(String message, Throwable exception) {
    if (cancellable != null) {
      cancellable.cancel(message, exception);
    } else {
      throw new UnsupportedOperationException("Streams cannot be cancelled that are produced by client. " +
          "Instead, server should reject incoming messages.");
    }
  }
  
  @Override
  StreamObserver<ArrowMessage> asObserver() {
    return new Observer();
  }
  
}
