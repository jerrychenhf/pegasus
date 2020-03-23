# User Guide
### Runtime requirements

* `HADOOP_HOME`: the root of your installed Hadoop distribution. Often has
`lib/native/libhdfs.so`.
* `JAVA_HOME`: the location of your Java SDK installation.
* `CLASSPATH`: must contain the Hadoop jars. You can set these using:
```shell
export CLASSPATH=`$HADOOP_HOME/bin/hadoop classpath --glob`
```
* `ARROW_LIBHDFS_DIR`: explicit location of `libhdfs.so` if it is
installed somewhere other than `$HADOOP_HOME/lib/native`

### Export PEGASUS_HOME
```
cd pegasus/cpp
export PEGASUS_HOME=`pwd`
```

## Start Planner

```
cd pegasus/bin
sh start-planner.sh --hostname=localhost --planner_port=30001
```

## Start Worker

```
cd pegasus/bin
sh start-worker.sh --hostname=localhost --worker_port=30002 --planner_hostname=localhost --planner_port=30001
```

### Spark Configurations for PEGASUS

Step 1. Make the following configuration changes in Spark configuration file `$SPARK_HOME/conf/spark-defaults.conf`. 
```
spark.files                       /home/pegasus/jars/pegasus-spark-datasource-1.0.0-SNAPSHOT-with-spark-3.0.0.jar     # absolute path of PEGASUS jar on your working node
spark.executor.extraClassPath     ./pegasus-spark-datasource-1.0.0-SNAPSHOT-with-spark-3.0.0.jar
                # relative path of PEGASUS jar
spark.driver.extraClassPath       /home/pegasus/jars/pegasus-spark-datasource-1.0.0-SNAPSHOT-with-spark-3.0.0.jar
     # absolute path of PEGASUS jar on your working node

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
