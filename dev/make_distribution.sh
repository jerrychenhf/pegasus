#!/bin/bash


set -e
START_PATH=$(pwd)



cd $START_PATH
cd ../cpp
mkdir -p build
cd build
cmake -DPEGASUS_USE_GLOG=ON ..
make

mkdir -p $START_PATH/target/pegasus
echo $(pwd)
#gather all
cp -f release/*.a $START_PATH/target/pegasus/
cp -f release/*.so $START_PATH/target/pegasus/
cp -f arrow_ep-install/libarrow.so.100 $START_PATH/target/pegasus/
cp -f arrow_ep-install/libarrow.so.100.0.0 $START_PATH/target/pegasus/
cp -f arrow_ep-install/libparquet.so.100 $START_PATH/target/pegasus/
cp -f arrow_ep-install/libparquet.so.100.0.0 $START_PATH/target/pegasus/
cp -f release/planner/plannerd $START_PATH/target/pegasus/
cp -f release/worker/workerd $START_PATH/target/pegasus/
cd $START_PATH/target
tar -czvf pegasus.tar.gz $START_PATH/target/*
