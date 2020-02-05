# Developer Guide

## Source Builds and Development
pegasus uses CMake as a build configuration system. We recommend building
out-of-source. If you are not familiar with this terminology:

* **In-source build**: ``cmake`` is invoked directly from the ``cpp``
  directory. This can be inflexible when you wish to maintain multiple build
  environments (e.g. one for debug builds and another for release builds)
* **Out-of-source build**: ``cmake`` is invoked from another directory,
  creating an isolated build environment that does not interact with any other
  build environment. For example, you could create ``cpp/build`` and
  invoke ``cmake $CMAKE_ARGS ..`` from this directory

### Prerequisites

#### Apache Arrow
You should have Apache Arrow C++ package installed in your building machine.

#### Building from Source Code
Refer to [Arrow Building document](https://arrow.apache.org/docs/developers/cpp.html#building) for the detail steps and prerequisties for building Arrow.

A few notes for building:
- Some CMake versions may cause download problems for dependancies on CentOS with http(s) proxy. On my CentOS 7 environment, both CMake 3.15.4 and CMake 3.10.2 caused download problem. The system default CMake version 3.7.1 works.
- Suggest to run as root user. Even "make" command needs "root" user for making and installing dependent libraries.

Here are simple steps of building and installing from source.

```
git clone https://github.com/Intel-bigdata/arrow-master.git -b pegasus-branch
cd arrow/cpp
mkdir build-arrow
cd build-arrow
yum install openssl-devel flex bison -y
cmake -DARROW_FLIGHT=ON -DARROW_PARQUET=ON -DARROW_HDFS=ON ..
make
make install
```

Setting up ARROW_HOME variable, add the following command to ~/.bashrc file.
```
export ARROW_HOME=/usr/local
```

### Building pegasus

```
git clone https://gitlab.devtools.intel.com/intel-bigdata/pegasus.git
cd pegasus/cpp
mkdir build
cd build
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DPEGASUS_USE_GLOG=ON ..
make
```

### Building C++ unit test
```
git clone https://gitlab.devtools.intel.com/intel-bigdata/pegasus.git
cd pegasus/cpp
mkdir debug
cd debug
cmake -DPEGASUS_BUILD_TESTS ..
make unittest
```
