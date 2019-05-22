#!/usr/bin/env bash

set -xue

root_dir=$PWD

rm -rf build
mkdir build
cd build
cmake ..
VERBOSE=1 cmake --build . --target install -- -j4

cd $root_dir
