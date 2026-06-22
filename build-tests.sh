#!/usr/bin/env sh

set -xue

cmake -B build-linux-tests

cmake --build build-linux-tests --target ia-test -- -j$(nproc)
