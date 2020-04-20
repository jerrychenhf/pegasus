#!/bin/bash

set -eu

rm -rf dep_tmp
mkdir dep_tmp
cd dep_tmp

# check cmake version

TARGET_CMAKE_VERSION=3.7.1

TARGET_CMAKE_SOURCE_URL=https://cmake.org/files/v3.7/cmake-3.7.1.tar.gz

CURRENT_CMAKE_VERSION_STR="$(cmake --version)"
echo ${CURRENT_CMAKE_VERSION_STR}
if [[ "$CURRENT_CMAKE_VERSION_STR" == "cmake version"* ]]; then
   echo "cmake is installed"
else
   echo "cmake is not installed"
   wget TARGET_CMAKE_SOURCE_URL
   tar xvf cmake-$TARGET_CMAKE_VERSION.tar.gz
   cd cmake-$TARGET_CMAKE_VERSION/
   ./bootstrap
   gmake
   gmake install
fi
array=(${CURRENT_CMAKE_VERSION_STR//,/ })

CURRENT_CMAKE_VERSION=${array[2]}

function version_lt() { test "$(echo "$@" | tr " " "\n" | sort -rV | head -n 1)" != "$1"; }
function version_ge() { test "$(echo "$@" | tr " " "\n" | sort -rV | head -n 1)" == "$1"; }

if version_lt $CURRENT_CMAKE_VERSION $TARGET_CMAKE_VERSION; then
   echo "$CURRENT_CMAKE_VERSION is less than $TARGET_CMAKE_VERSION"
   wget TARGET_CMAKE_SOURCE_URL
   tar xvf cmake-$TARGET_CMAKE_VERSION.tar.gz
   cd cmake-$TARGET_CMAKE_VERSION/
   ./bootstrap
   gmake
   gmake install
   yum remove cmake -y
   ln -s /usr/local/bin/cmake /usr/bin/
   cd ..
fi

if version_ge $CURRENT_CMAKE_VERSION $TARGET_CMAKE_VERSION; then
   echo "$VERSION is greater than or equal to $TARGET_CMAKE_VERSION"
fi

# check dependencies arrow
# install ARROW
if [ ! -d "/usr/local/include/arrow" ]; then
   git clone https://github.com/Intel-bigdata/arrow.git -b branch-1.0-pegasus
   cd arrow/cpp
   mkdir build-arrow
   cd build-arrow
   yum install openssl-devel flex bison -y
   cmake -DARROW_FLIGHT=ON -DARROW_PARQUET=ON -DARROW_HDFS=ON -DARROW_WITH_SNAPPY=ON -DARROW_BUILD_TESTS=ON ..
   make
   make install
   cp -r $(pwd)/src/arrow/filesystem /usr/local/include/arrow/
   echo "export ARROW_HOME=/usr/local" >env.sh
   cd ../java
   mvn -DskipTests clean instal
fi

# offline build


cd ../../cpp
mkdir build
cd build
cmake -DPEGASUS_USE_GLOG=ON ..
make
