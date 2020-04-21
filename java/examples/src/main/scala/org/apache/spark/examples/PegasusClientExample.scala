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

package org.apache.spark.examples

import scala.collection.JavaConverters._
import org.apache.pegasus.rpc.{FlightDescriptor, Location, Ticket}
import org.apache.spark.sql.execution.datasources.v2.pegasus.PegasusClientFactory
import java.util.HashMap

import org.apache.spark.sql.vectorized.{ArrowColumnVector, ColumnVector, ColumnarBatch}

object PegasusClientExample {

    def main(args: Array[String]): Unit = {

        val plannerClientFactory = new PegasusClientFactory(
              Location.forGrpcInsecure("bdpe611n3", 30001), null, null)
        val plannerClient = plannerClientFactory.apply
        val path = "hdfs://10.239.47.55:9000/genData1000/catalog_sales"
        val properties = new HashMap[String, String]
        properties.put("table.location", path)
        properties.put("provider", "SPARK")
        properties.put("format", "PARQUET")
        val pathList = Seq(path)
        val descriptor = FlightDescriptor.path(pathList.asJava, properties)

        val plannerBegin = System.currentTimeMillis

        val info = plannerClient.getInfo(descriptor)
        plannerClient.close()
        plannerClientFactory.close()

        val plannerEnd = System.currentTimeMillis
        System.out.println("Planner Time: " + (plannerEnd - plannerBegin));

        val endpoints = info.getEndpoints.asScala
        val endpoint =  endpoints.asJava.get(0)
        val ticket = endpoint.getTicket
        val locations = endpoint.getLocations.asScala
        val location = locations(0)

        val workerBegin = System.currentTimeMillis

        val workerClientFactory = new PegasusClientFactory(location, null, null)
        val workerClient = workerClientFactory.apply
        val stream = workerClient.getStream(ticket)
        var num = 0
        while (stream.next()) {
            val vectors = stream.getRoot.getFieldVectors
            val columns = vectors.asScala.map { vector =>
              new ArrowColumnVector(vector).asInstanceOf[ColumnVector]
            }.toArray
            val batch = new ColumnarBatch(columns)
            batch.setNumRows(stream.getRoot().getRowCount())
            num = num + 1
        }
        System.out.println("batch num: " + num)
        stream.close()
        workerClient.close()
        workerClientFactory.close()

        val workerEnd = System.currentTimeMillis
        System.out.println("Worker Time: " + (workerEnd - workerBegin));
    }
}