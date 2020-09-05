#!/usr/bin/env sh

set -xue

root_dir=$PWD

rm -rf build
mkdir build
cd build
cmake ..
VERBOSE=1 cmake --build . --target install -- -j2

cd $root_dir
