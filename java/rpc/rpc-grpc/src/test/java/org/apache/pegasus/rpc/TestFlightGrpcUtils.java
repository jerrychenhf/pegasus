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

import static org.junit.jupiter.api.Assertions.assertThrows;

import java.io.IOException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import org.apache.arrow.memory.BufferAllocator;
import org.apache.arrow.memory.RootAllocator;
import org.apache.pegasus.rpc.auth.ServerAuthHandler;
import org.junit.Assert;
import org.junit.Test;

import com.google.protobuf.Empty;

import io.grpc.BindableService;
import io.grpc.ManagedChannel;
import io.grpc.Server;
import io.grpc.inprocess.InProcessChannelBuilder;
import io.grpc.inprocess.InProcessServerBuilder;
import io.grpc.stub.StreamObserver;

/**
 * Unit test which adds 2 services to same server end point.
 */
public class TestFlightGrpcUtils {

  /**
   * This test checks if multiple gRPC services can be added to the same
   * server endpoint and if they can be used by different clients via the same channel.
   * @throws IOException If server fails to start.
   */
  @Test
  public void testMultipleGrpcServices() throws IOException {

    //Defines flight service
    final BufferAllocator allocator = new RootAllocator(Integer.MAX_VALUE);
    final NoOpFlightProducer producer = new NoOpFlightProducer();
    final ServerAuthHandler authHandler = ServerAuthHandler.NO_OP;
    final ExecutorService exec = Executors.newCachedThreadPool();
    final BindableService flightBindingService = FlightGrpcUtils.createFlightService(allocator, producer,
            authHandler, exec);

    //initializes server with 2 services - FlightBindingService & TestService
    final String serverName = InProcessServerBuilder.generateName();
    final Server server = InProcessServerBuilder.forName(serverName)
            .directExecutor()
            .addService(flightBindingService)
            .addService(new TestServiceAdapter())
            .build();
    server.start();

    //Initializes channel so that multiple clients can communicate with server
    final ManagedChannel managedChannel = InProcessChannelBuilder.forName(serverName)
            .directExecutor()
            .build();

    //Defines flight client and calls service method. Since we use a NoOpFlightProducer we expect the service
    //to throw a RunTimeException
    final FlightClient flightClient = FlightGrpcUtils.createFlightClient(allocator, managedChannel);
    final Iterable<ActionType> actionTypes = flightClient.listActions();
    assertThrows(FlightRuntimeException.class, () -> actionTypes.forEach(
        actionType -> System.out.println(actionType.toString())));

    //Define Test client as a blocking stub and call test method which correctly returns an empty protobuf object
    final TestServiceGrpc.TestServiceBlockingStub blockingStub = TestServiceGrpc.newBlockingStub(managedChannel);
    Assert.assertEquals(Empty.newBuilder().build(), blockingStub.test(Empty.newBuilder().build()));
  }

  /**
   * Private class used for testing purposes that overrides service behavior.
   */
  private class TestServiceAdapter extends TestServiceGrpc.TestServiceImplBase {

    /**
     * gRPC service that receives an empty object & returns and empty protobuf object.
     * @param request google.protobuf.Empty
     * @param responseObserver google.protobuf.Empty
     */
    @Override
    public void test(Empty request, StreamObserver<Empty> responseObserver) {
      responseObserver.onNext(Empty.newBuilder().build());
      responseObserver.onCompleted();
    }
  }
}

