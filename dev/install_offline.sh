#!/bin/bash

#set -eu

START_PATH=$(pwd)
rm -rf dep_tmp
mkdir dep_tmp
cd dep_tmp

# check cmake version

TARGET_CMAKE_VERSION=3.7.1

TARGET_CMAKE_SOURCE_URL=https://cmake.org/files/v3.7/cmake-3.7.1.tar.gz

CURRENT_CMAKE_VERSION_STR="$(cmake --version)"
set -eu
echo ${CURRENT_CMAKE_VERSION_STR}
if [[ "$CURRENT_CMAKE_VERSION_STR" == "cmake version"* ]]; then
   echo "cmake is installed"
else
   echo "cmake is not installed"
   wget ${TARGET_CMAKE_SOURCE_URL}
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

cd START_PATH
cd ../thirdparty/


./download_dependencies.sh  > pegasus_env.conf
./download_arrow_dependencies.sh  > arrow_env.conf

cd ../cpp
mkdir build
cd build
cmake -DPEGASUS_USE_GLOG=ON ..
make
