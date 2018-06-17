#!/bin/bash

set -xue

root_dir=$PWD

rm -rf build
mkdir build
cd build
cmake ..
make -j 4 install

cd $root_dir
