#!/bin/bash -e

START_DIR="$(pwd)"

mkdir -p cmake-build/macosx-workstation/ && cd cmake-build/macosx-workstation/
cmake -G Xcode ../../src/cpp
xcodebuild -target test_buffer -configuration Debug

#ctest -C Debug -VV | h '^.*failed.*$'
ctest -C Debug -VV

cd $START_DIR

