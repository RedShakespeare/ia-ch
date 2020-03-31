#!/usr/bin/env bash

set -xue

root_dir=$PWD

mkdir -p build
cd build
cmake ..
cmake --build . --target ia-test -- -j2

cd $root_dir
