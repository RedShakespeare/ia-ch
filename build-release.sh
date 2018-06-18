#!/bin/bash

set -xue

root_dir=$PWD

rm -rf build
mkdir build
cd build
cmake ..
cmake --build . --target install -- -j 4

cd $root_dir
