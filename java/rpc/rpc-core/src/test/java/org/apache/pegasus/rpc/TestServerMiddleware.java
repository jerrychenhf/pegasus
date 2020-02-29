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
import java.util.Collections;
import java.util.List;
import java.util.concurrent.CompletableFuture;
import java.util.function.BiConsumer;

import org.apache.arrow.memory.BufferAllocator;
import org.apache.arrow.memory.RootAllocator;
import org.apache.arrow.vector.VectorSchemaRoot;
import org.apache.arrow.vector.types.pojo.Schema;
import org.apache.pegasus.rpc.FlightClient.ClientStreamListener;
import org.apache.pegasus.rpc.FlightServerMiddleware.Factory;
import org.apache.pegasus.rpc.FlightServerMiddleware.Key;
import org.junit.Assert;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class TestServerMiddleware {

  private static final RuntimeException EXPECTED_EXCEPTION = new RuntimeException("test");

  /**
   * Make sure errors in DoPut are intercepted.
   */
  @Test
  public void doPutErrors() {
    test(
        new ErrorProducer(EXPECTED_EXCEPTION),
        (allocator, client) -> {
          final FlightDescriptor descriptor = FlightDescriptor.path("test");
          try (final VectorSchemaRoot root = VectorSchemaRoot.create(new Schema(Collections.emptyList()), allocator)) {
            final ClientStreamListener listener = client.startPut(descriptor, root, new SyncPutListener());
            listener.completed();
            FlightTestUtil.assertCode(FlightStatusCode.INTERNAL, listener::getResult);
          }
        }, (recorder) -> {
          final CallStatus status = recorder.statusFuture.get();
          Assert.assertNotNull(status);
          Assert.assertNotNull(status.cause());
          Assert.assertEquals(FlightStatusCode.INTERNAL, status.code());
        });
    // Check the status after server shutdown (to make sure gRPC finishes pending calls on the server side)
  }

  /**
   * Make sure custom error codes in DoPut are intercepted.
   */
  @Test
  public void doPutCustomCode() {
    test(
        new ErrorProducer(CallStatus.UNAVAILABLE.withDescription("description").toRuntimeException()),
        (allocator, client) -> {
          final FlightDescriptor descriptor = FlightDescriptor.path("test");
          try (final VectorSchemaRoot root = VectorSchemaRoot.create(new Schema(Collections.emptyList()), allocator)) {
            final ClientStreamListener listener = client.startPut(descriptor, root, new SyncPutListener());
            listener.completed();
            FlightTestUtil.assertCode(FlightStatusCode.UNAVAILABLE, listener::getResult);
          }
        }, (recorder) -> {
          final CallStatus status = recorder.statusFuture.get();
          Assert.assertNotNull(status);
          Assert.assertNull(status.cause());
          Assert.assertEquals(FlightStatusCode.UNAVAILABLE, status.code());
          Assert.assertEquals("description", status.description());
        });
  }

  /**
   * Make sure uncaught exceptions in DoPut are intercepted.
   */
  @Test
  public void doPutUncaught() {
    test(new ServerErrorProducer(EXPECTED_EXCEPTION),
        (allocator, client) -> {
          final FlightDescriptor descriptor = FlightDescriptor.path("test");
          try (final VectorSchemaRoot root = VectorSchemaRoot.create(new Schema(Collections.emptyList()), allocator)) {
            final ClientStreamListener listener = client.startPut(descriptor, root, new SyncPutListener());
            listener.completed();
            listener.getResult();
          }
        }, (recorder) -> {
          final CallStatus status = recorder.statusFuture.get();
          final Throwable err = recorder.errFuture.get();
          Assert.assertNotNull(status);
          Assert.assertEquals(FlightStatusCode.OK, status.code());
          Assert.assertNull(status.cause());
          Assert.assertNotNull(err);
          Assert.assertEquals(EXPECTED_EXCEPTION.getMessage(), err.getMessage());
        });
  }

  @Test
  public void listFlightsUncaught() {
    test(new ServerErrorProducer(EXPECTED_EXCEPTION),
        (allocator, client) -> client.listFlights(new Criteria(new byte[0])).forEach((action) -> {
        }), (recorder) -> {
          final CallStatus status = recorder.statusFuture.get();
          final Throwable err = recorder.errFuture.get();
          Assert.assertNotNull(status);
          Assert.assertEquals(FlightStatusCode.OK, status.code());
          Assert.assertNull(status.cause());
          Assert.assertNotNull(err);
          Assert.assertEquals(EXPECTED_EXCEPTION.getMessage(), err.getMessage());
        });
  }

  @Test
  public void doActionUncaught() {
    test(new ServerErrorProducer(EXPECTED_EXCEPTION),
        (allocator, client) -> client.doAction(new Action("test")).forEachRemaining(result -> {
        }), (recorder) -> {
          final CallStatus status = recorder.statusFuture.get();
          final Throwable err = recorder.errFuture.get();
          Assert.assertNotNull(status);
          Assert.assertEquals(FlightStatusCode.OK, status.code());
          Assert.assertNull(status.cause());
          Assert.assertNotNull(err);
          Assert.assertEquals(EXPECTED_EXCEPTION.getMessage(), err.getMessage());
        });
  }

  @Test
  public void listActionsUncaught() {
    test(new ServerErrorProducer(EXPECTED_EXCEPTION),
        (allocator, client) -> client.listActions().forEach(result -> {
        }), (recorder) -> {
          final CallStatus status = recorder.statusFuture.get();
          final Throwable err = recorder.errFuture.get();
          Assert.assertNotNull(status);
          Assert.assertEquals(FlightStatusCode.OK, status.code());
          Assert.assertNull(status.cause());
          Assert.assertNotNull(err);
          Assert.assertEquals(EXPECTED_EXCEPTION.getMessage(), err.getMessage());
        });
  }

  @Test
  public void getFlightInfoUncaught() {
    test(new ServerErrorProducer(EXPECTED_EXCEPTION),
        (allocator, client) -> {
          FlightTestUtil.assertCode(FlightStatusCode.INTERNAL, () -> client.getInfo(FlightDescriptor.path("test")));
        }, (recorder) -> {
          final CallStatus status = recorder.statusFuture.get();
          Assert.assertNotNull(status);
          Assert.assertEquals(FlightStatusCode.INTERNAL, status.code());
          Assert.assertNotNull(status.cause());
          Assert.assertEquals(EXPECTED_EXCEPTION.getMessage(), status.cause().getMessage());
        });
  }

  @Test
  public void doGetUncaught() {
    test(new ServerErrorProducer(EXPECTED_EXCEPTION),
        (allocator, client) -> {
          try (final FlightStream stream = client.getStream(new Ticket(new byte[0], new byte[0], new byte[0], null))) {
            while (stream.next()) {
            }
          } catch (Exception e) {
            Assert.fail(e.toString());
          }
        }, (recorder) -> {
          final CallStatus status = recorder.statusFuture.get();
          final Throwable err = recorder.errFuture.get();
          Assert.assertNotNull(status);
          Assert.assertEquals(FlightStatusCode.OK, status.code());
          Assert.assertNull(status.cause());
          Assert.assertNotNull(err);
          Assert.assertEquals(EXPECTED_EXCEPTION.getMessage(), err.getMessage());
        });
  }

  /**
   * A middleware that records the last error on any call.
   */
  static class ErrorRecorder implements FlightServerMiddleware {

    CompletableFuture<CallStatus> statusFuture = new CompletableFuture<>();
    CompletableFuture<Throwable> errFuture = new CompletableFuture<>();

    @Override
    public void onBeforeSendingHeaders(CallHeaders outgoingHeaders) {
    }

    @Override
    public void onCallCompleted(CallStatus status) {
      statusFuture.complete(status);
    }

    @Override
    public void onCallErrored(Throwable err) {
      errFuture.complete(err);
    }

    static class Factory implements FlightServerMiddleware.Factory<ErrorRecorder> {

      ErrorRecorder instance = new ErrorRecorder();

      @Override
      public ErrorRecorder onCallStarted(CallInfo info, CallHeaders incomingHeaders) {
        return instance;
      }
    }
  }

  /**
   * A producer that throws the given exception on a call.
   */
  static class ErrorProducer extends NoOpFlightProducer {

    final RuntimeException error;

    ErrorProducer(RuntimeException t) {
      error = t;
    }

    @Override
    public Runnable acceptPut(CallContext context, FlightStream flightStream, StreamListener<PutResult> ackStream) {
      return () -> {
        // Drain queue to avoid FlightStream#close cancelling the call
        while (flightStream.next()) {
        }
        throw error;
      };
    }
  }

  /**
   * A producer that throws the given exception on a call, but only after sending a success to the client.
   */
  static class ServerErrorProducer extends NoOpFlightProducer {

    final RuntimeException error;

    ServerErrorProducer(RuntimeException t) {
      error = t;
    }

    @Override
    public void getStream(CallContext context, Ticket ticket, ServerStreamListener listener) {
      try (final BufferAllocator allocator = new RootAllocator(Integer.MAX_VALUE);
          final VectorSchemaRoot root = VectorSchemaRoot.create(new Schema(Collections.emptyList()), allocator)) {
        listener.start(root);
        listener.completed();
      }
      throw error;
    }

    @Override
    public void listFlights(CallContext context, Criteria criteria, StreamListener<FlightInfo> listener) {
      listener.onCompleted();
      throw error;
    }

    @Override
    public FlightInfo getFlightInfo(CallContext context, FlightDescriptor descriptor) {
      throw error;
    }

    @Override
    public Runnable acceptPut(CallContext context, FlightStream flightStream, StreamListener<PutResult> ackStream) {
      return () -> {
        while (flightStream.next()) {
        }
        ackStream.onCompleted();
        throw error;
      };
    }

    @Override
    public void doAction(CallContext context, Action action, StreamListener<Result> listener) {
      listener.onCompleted();
      throw error;
    }

    @Override
    public void listActions(CallContext context, StreamListener<ActionType> listener) {
      listener.onCompleted();
      throw error;
    }
  }

  static class ServerMiddlewarePair<T extends FlightServerMiddleware> {

    final FlightServerMiddleware.Key<T> key;
    final FlightServerMiddleware.Factory<T> factory;

    ServerMiddlewarePair(Key<T> key, Factory<T> factory) {
      this.key = key;
      this.factory = factory;
    }
  }

  /**
   * Spin up a service with the given middleware and producer.
   *
   * @param producer The Flight producer to use.
   * @param middleware A list of middleware to register.
   * @param body A function to run as the body of the test.
   * @param <T> The middleware type.
   */
  static <T extends FlightServerMiddleware> void test(FlightProducer producer, List<ServerMiddlewarePair<T>> middleware,
      BiConsumer<BufferAllocator, FlightClient> body) {
    try (final BufferAllocator allocator = new RootAllocator(Integer.MAX_VALUE)) {
      final FlightServer server = FlightTestUtil
          .getStartedServer(location -> {
            final FlightServer.Builder builder = FlightServer.builder(allocator, location, producer);
            middleware.forEach(pair -> builder.middleware(pair.key, pair.factory));
            return builder.build();
          });
      try (final FlightServer ignored = server;
          final FlightClient client = FlightClient.builder(allocator, server.getLocation()).build()
      ) {
        body.accept(allocator, client);
      }
    } catch (InterruptedException | IOException e) {
      throw new RuntimeException(e);
    }
  }

  static void test(FlightProducer producer, BiConsumer<BufferAllocator, FlightClient> body,
      ErrorConsumer<ErrorRecorder> verify) {
    final ErrorRecorder.Factory factory = new ErrorRecorder.Factory();
    final List<ServerMiddlewarePair<ErrorRecorder>> middleware = Collections
        .singletonList(new ServerMiddlewarePair<>(Key.of("m"), factory));
    test(producer, middleware, (allocator, client) -> {
      body.accept(allocator, client);
      try {
        verify.accept(factory.instance);
      } catch (Exception e) {
        throw new RuntimeException(e);
      }
    });
  }

  @FunctionalInterface
  interface ErrorConsumer<T> {
    void accept(T obj) throws Exception;
  }
}
