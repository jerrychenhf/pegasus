# Benchmark Guide

### Run iperf
#### Start server
```
iperf -s
```
#### Run
-P15 is the 15 threads
```
iperf -c localhost -p 5001 -P15
```

### Run Arrow Flight Benchmark
#### Build Arrow
```
git clone https://github.com/Intel-bigdata/arrow.git -b branch-1.0-pegasus
cd arrow/cpp
mkdir build-arrow
cd build-arrow
yum install openssl-devel flex bison -y
cmake -DARROW_FLIGHT=ON -DARROW_PARQUET=ON -DARROW_HDFS=ON -DARROW_WITH_SNAPPY=ON -DARROW_BUILD_TESTS=ON -DARROW_BUILD_BENCHMARKS=ON ..
make
```
#### Run Benchmark
```
cd release
```
```
./arrow-flight-benchmark --server_host localhost --num_streams 1 --num_threads 1
```

### Run Arrow Flight Benchmark Client with Pegasus Server
#### Building Pegasus C++
```
git clone https://gitlab.devtools.intel.com/intel-bigdata/pegasus.git
cd pegasus/cpp
mkdir build
cd build
cmake -DPEGASUS_USE_GLOG=ON ..
make
```
#### Start Planner and Worker
```
cd pegasus/bin
sh start-planner.sh --planner_hostname=localhost --planner_port=30001
```
```
cd pegasus/bin
sh start-worker.sh --worker_hostname=localhost --worker_port=30002 --planner_hostname=localhost --planner_port=30001
```
#### Run Benchmark
```
cd release/benchmark/
./benchmark --server_host=localhost
```

### Run Java client Driver with Pegasus Server
10 is the thread number.
```
cd pegasus/java
./bin/run-example org.apache.spark.examples.PegasusClientExample 10

```
