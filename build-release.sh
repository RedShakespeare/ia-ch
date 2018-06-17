#!/bin/bash

set -xue

root_dir=$PWD

mkdir -p build
cd build
cmake ..
make -j 4 ia

cd $root_dir
