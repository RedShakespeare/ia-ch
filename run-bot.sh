#!/usr/bin/env bash

set -xue

root_dir=$PWD

./build-debug.sh

cd build

./ia-debug --bot

cd $root_dir
