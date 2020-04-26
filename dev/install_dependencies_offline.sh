#!/bin/bash

#set -eu

START_PATH=$(pwd)

# check cmake version

TARGET_CMAKE_VERSION=3.7.1

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
    echo "$START_PATH/thirdparty/cmake-$CMAKE_TARGET_VERSION.tar.gz"
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

prepare_cmake

cd $START_PATH
cd ../thirdparty/

./download_dependencies.sh  > pegasus_env.conf
./download_arrow_dependencies.sh  > arrow_env.conf

source pegasus_env.conf
source arrow_env.conf

current_path=${pwd}
echo "Please cd $current_path  source pegasus_env.conf and arrow_env.conf.Then run make_distribution.sh"
