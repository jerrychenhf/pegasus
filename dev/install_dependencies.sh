#!/bin/bash

#set -eu

START_PATH=$(pwd)


# check cmake version
CMAKE_TARGET_VERSION=3.7.1
CMAKE_MIN_VERSION=3.7
TARGET_CMAKE_SOURCE_URL=https://cmake.org/files/v3.7/cmake-3.7.1.tar.gz

function version_lt() { test "$(echo "$@" | tr " " "\n" | sort -rV | head -n 1)" != "$1"; }
function version_ge() { test "$(echo "$@" | tr " " "\n" | sort -rV | head -n 1)" == "$1"; }

function prepare_cmake() {
  CURRENT_CMAKE_VERSION_STR="$(cmake --version)"
  cd $START_PATH

  if [[ "$CURRENT_CMAKE_VERSION_STR" == "cmake version"* ]]; then
    echo "cmake is installed"
    array=(${CURRENT_CMAKE_VERSION_STR//,/ })
    CURRENT_CMAKE_VERSION=${array[2]}
    if version_lt $CURRENT_CMAKE_VERSION $CMAKE_MIN_VERSION; then
      echo "$CURRENT_CMAKE_VERSION is less than $CMAKE_MIN_VERSION,install cmake $CMAKE_TARGET_VERSION"
      mkdir -p thirdparty
      cd thirdparty
      echo "$START_PATH/thirdparty/cmake-$CMAKE_TARGET_VERSION.tar.gz"
      if [ ! -f "$START_PATH/thirdparty/cmake-$CMAKE_TARGET_VERSION.tar.gz" ]; then
        wget $TARGET_CMAKE_SOURCE_URL
      fi
      tar xvf cmake-$CMAKE_TARGET_VERSION.tar.gz
      cd cmake-$CMAKE_TARGET_VERSION/
      ./bootstrap
      gmake
      gmake install
      yum remove cmake -y
      ln -s /usr/local/bin/cmake /usr/bin/
      cd $START_PATH
    fi
  else
    echo "cmake is not installed"
    mkdir -p thirdparty
    cd thirdparty
    if [ ! -f "cmake-$CMAKE_TARGET_VERSION.tar.gz" ]; then
      wget $TARGET_CMAKE_SOURCE_URL
    fi

    tar xvf cmake-$CMAKE_TARGET_VERSION.tar.gz
    cd cmake-$CMAKE_TARGET_VERSION/
    ./bootstrap
    gmake
    gmake install
    cd $START_PATH
  fi
}

# check dependencies arrow
# install ARROW
function prepare_arrow(){
   if [ ! -d "/usr/local/include/arrow" ]; then
      git clone https://github.com/Intel-bigdata/arrow.git -b branch-1.0-pegasus
      cd arrow/cpp
      mkdir -p build-arrow
      cd build-arrow
      yum install openssl-devel flex bison -y
      cmake -DARROW_FLIGHT=ON -DARROW_PARQUET=ON -DARROW_HDFS=ON -DARROW_WITH_SNAPPY=ON -DARROW_BUILD_TESTS=ON ..
      make
      make install
      cp -r $(pwd)/src/arrow/filesystem /usr/local/include/arrow/
      cd ../java
      mvn -DskipTests clean instal
      cd $START_PATH
      echo "export ARROW_HOME=/usr/local" >env.sh
   fi
}


echo "Please source env.sh ,then run make_distribution.sh"
