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
sh start_planner.sh --hostname=localhost --planner_port=30001
```

## Start Worker

```
cd pegasus/bin
sh start_worker.sh --hostname=localhost --worker_port=30002 --planner_hostname=localhost --planner_port=30001
```