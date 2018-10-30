#!/bin/bash

set -xue

root_dir=$PWD

mkdir -p build
cd build
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 ..
cmake --build . --target ia-debug -- -j4

cd $root_dir
