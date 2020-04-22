# User Guide
### Runtime requirements
By default, the HDFS client C++ class uses the libhdfs JNI
interface to the Java Hadoop client. This library is loaded **at runtime**
(rather than at link / library load time, since the library may not be in your
LD_LIBRARY_PATH), and relies on some environment variables.

* `HADOOP_HOME`: the root of your installed Hadoop distribution. Often has
`lib/native/libhdfs.so`.
* `JAVA_HOME`: the location of your Java SDK installation.
* `CLASSPATH`: must contain the Hadoop jars. You can set these using:
```shell
export CLASSPATH=`$HADOOP_HOME/bin/hadoop classpath --glob`
```
* `ARROW_LIBHDFS_DIR` (optional): explicit location of `libhdfs.so` if it is
installed somewhere other than `$HADOOP_HOME/lib/native`

### Export PEGASUS_HOME
```
cd pegasus/cpp
export PEGASUS_HOME=`pwd`
```

## Start Planner

```
cd pegasus/bin
sh start-planner.sh --planner_hostname=localhost --planner_port=30001
```

## Start Worker

```
cd pegasus/bin
sh start-worker.sh --worker_hostname=localhost --worker_port=30002 --planner_hostname=localhost --planner_port=30001
```

### Cluster Launch Scripts
To launch a Pegasus cluster with the launch scripts, you should create a file called conf/workers in your Pegasus directory, which must contain the hostnames of all the machines where you intend to start Pegasus workers, one per line. If conf/workers does not exist, the launch scripts defaults to a single machine (localhost). And copy pegasus folder to all your worker machines.

Note, the planner machine accesses each of the worker machines via ssh. By default, ssh is run in parallel and requires password-less (using a private key) access to be setup.

Once you’ve set up this file, you can launch or stop your cluster with the following shell scripts, available in PEGASUS_HOME/bin:

bin/start-planner.sh - Starts a planner instance on the machine the script is executed on.
bin/start-workers.sh - Starts a worker instance on each machine specified in the conf/workers file.
bin/start-worker.sh - Starts a worker instance on the machine the script is executed on.
bin/start-all.sh - Starts both a planner and a number of workers as described above.
bin/stop-planner.sh - Stops the planner that was started via the bin/start-master.sh script.
bin/stop-workers.sh - Stops all workers instances on the machines specified in the conf/workers file.
bin/stop-all.sh - Stops both the planner and the workers as described above.

You can optionally configure the cluster further by setting environment variables in conf/pegasus-env.sh. The following settings are available:
```
PEGASUS_PLANNER_PORT   start the planner on a different port (default: 30001).
PEGASUS_WORKER_PORT    start the worker on a different port (default: 30002).
```

### Start PLanner and Worker with debug build
```
cd pegasus
sh bin/start-planner.sh --build_type=debug
sh bin/start-worker.sh --build_type=debug
```
or
```
cd pegasus
sh bin/start-all.sh --build_type=debug
```

### Spark Configurations for PEGASUS

Step 1. Make the following configuration changes in Spark configuration file `$SPARK_HOME/conf/spark-defaults.conf`. 
```
spark.files                       /home/pegasus/jars/pegasus-spark-datasource-1.0.0-SNAPSHOT-with-spark-3.0.0.jar # absolute path of PEGASUS jar on your working node
spark.executor.extraClassPath     ./pegasus-spark-datasource-1.0.0-SNAPSHOT-with-spark-3.0.0.jar # relative path of PEGASUS jar
spark.driver.extraClassPath       /home/pegasus/jars/pegasus-spark-datasource-1.0.0-SNAPSHOT-with-spark-3.0.0.jar # absolute path of PEGASUS jar on your working node

```
Step 2. Launch Spark ***ThriftServer***

After configuration, you can launch Spark Thift Server. And use Beeline command line tool to connect to the Thrift Server to execute DDL or DML operations. In production, Spark Thrift Server will have its own metastore database directory or metastore service and use DDL's  through Beeline for creating your tables.

Execute the following command to launch Thrift JDBC server.
```
. $SPARK_HOME/sbin/start-thriftserver.sh
```
Step3. Use Beeline and connect to the Thrift JDBC server using the following command, replacing the hostname (mythriftserver) with your own Thrift Server hostname.

```
./beeline -u jdbc:hive2://mythriftserver:10000       
```
After the connection is established, execute the following command to check the metastore is initialized correctly.

```
> SHOW databases;
> USE default;
> SHOW tables;
```
 
Step 4. Run query to create table. For example,

```
> CREATE TABLE pegasusitem USING PARQUET LOCATION 'hdfs://10.239.47.55:9000/genData1000/item';
```

Step 5. Run queries on table. For example,

```
> SELECT * FROM pegasusitem;
```
