#!/usr/bin/env bash

set -xue

root_dir=$PWD

rm -rf build
mkdir build
cd build
cmake -DWIN32=TRUE -DMSVC=FALSE -DARCH=64bit -DCMAKE_TOOLCHAIN_FILE=../Toolchain-cross-mingw32.txt ..
VERBOSE=1 cmake --build . --target install -- -j4

cd $root_dir
