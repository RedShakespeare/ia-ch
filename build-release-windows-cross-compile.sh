#!/bin/bash

set -xue

root_dir=$PWD

rm -rf build
mkdir build
cd build
cmake -DWIN32=TRUE -DMSVC=FALSE -DARCH=64bit -DCMAKE_TOOLCHAIN_FILE=../Toolchain-cross-mingw32.txt ..
cmake --build . --target install -- -j 4

cd $root_dir
