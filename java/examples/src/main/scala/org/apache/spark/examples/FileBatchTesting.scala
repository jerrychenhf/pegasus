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

import java.util.HashMap

import org.apache.pegasus.rpc.{FlightDescriptor, FlightFileBatchStream, FlightInfo, Location}
import org.apache.spark.sql.execution.datasources.v2.pegasus.PegasusClientFactory

import scala.collection.JavaConverters._

object FileBatchTesting {

    def main(args: Array[String]): Unit = {

        val plannerClientFactory = new PegasusClientFactory(
              Location.forGrpcInsecure("127.0.0.1", 30001), null, null)
        val plannerClient = plannerClientFactory.apply(true)
         // val path = "hdfs://10.239.47.55:9000/genData10/store_sales"
        val path = "hdfs://10.239.47.55:9000/genData10/customer"
        val properties = new HashMap[String, String]
        properties.put(FlightDescriptor.TABLE_LOCATION , path)
        properties.put(FlightDescriptor.CATALOG_PROVIDER , FlightDescriptor.CATALOG_PROVIDER_SPARK)
        properties.put(FlightDescriptor.FILE_FORMAT, FlightDescriptor.FILE_FORMAT_PARQUET)
       //  properties.put(FlightDescriptor.COLUMN_NAMES, "ss_sold_date_sk, ss_item_sk")
        properties.put(FlightDescriptor.COLUMN_NAMES, "c_customer_sk")
       // properties.put(FlightDescriptor.COLUMN_NAMES, "c_customer_id")
      //  properties.put(FlightDescriptor.COLUMN_NAMES, "c_customer_sk, c_customer_id")
        val pathList = Seq(path)
        val descriptor = FlightDescriptor.path(pathList.asJava, properties)

        val plannerBegin = System.currentTimeMillis

        val info: FlightInfo = plannerClient.getInfo(descriptor)
        plannerClient.close()
        plannerClientFactory.close()

        val plannerEnd = System.currentTimeMillis
        System.out.println("Planner Time: " + (plannerEnd - plannerBegin));

        val endpoints = info.getEndpoints.asScala
        val threadNum = {
          if (args.length > 0)
            args(0).toInt
          else 1
        }
        val start = {
          if (args.length > 1)
            args(1).toInt
          else 0
        }

        val begin = System.currentTimeMillis
        val workerBegin = System.currentTimeMillis
        System.out.println("endpoint index: " + 0)
        var endpoint = endpoints.asJava.get(0)
        val ticket = endpoint.getTicket
        val locations = endpoint.getLocations.asScala
        val location = locations(0)
        System.out.println("location: " + location)
        val workerClientFactory = new PegasusClientFactory(location, null, null)
        val workerClient = workerClientFactory.apply(true)
        val stream = workerClient.getStream(ticket)
        var num = 0
        while (stream.next()) {
          val byteBuf = stream.asInstanceOf[FlightFileBatchStream].getFileBatchRoot().getbyteBuffers()

//          val bodyarrowBuf = arrowbuf.get(0)
//          bodyarrowBuf.getReferenceManager.retain()
//          val bodydata: Array[Byte] = new Array[Byte](bodyarrowBuf.capacity.toInt)
//          bodyarrowBuf.getBytes(0, bodydata)
//          bodyarrowBuf.getReferenceManager.release

          val bodyBuf = byteBuf.get(0)
          System.out.println("bodyBuf.capacity(): " + bodyBuf.capacity())
          System.out.println("bodyBuf.array().length: " + bodyBuf.array().length)
          System.out.println("bodyBuf.remaining: " + bodyBuf.remaining())

          val bytearr = bodyBuf.array()
          for(i <- 0 until 100){
            System.out.println(i + ":" +  bytearr(i))
          }

//          val bodydata: Array[Byte] = new Array[Byte](bodyBuf.array().length)
//          bodyBuf.get(bodydata)



          //                        val vectors = stream.getRoot.getFieldVectors
          //                        val columns = vectors.asScala.map { vector =>
          //                            new ArrowColumnVector(vector).asInstanceOf[ColumnVector]
          //                        }.toArray
          //                        val batch = new ColumnarBatch(columns)
          //                        batch.setNumRows(stream.getRoot().getRowCount())
          num = num + 1
          //                        System.out.println("batch.numRows(): " + batch.numRows())
        }
        System.out.println("batch num: " + num)
        stream.close()
        workerClient.close()
        workerClientFactory.close()

        val workerEnd = System.currentTimeMillis
        System.out.println("Worker Time: " + (workerEnd - workerBegin));


      val end = System.currentTimeMillis
      System.out.println("Worker total Time: " + (end - begin));
    }
}