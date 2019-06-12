#!/usr/bin/env bash

set -xue

root_dir=$PWD

./build-test.sh

cd build

ctest --verbose

cd $root_dir
