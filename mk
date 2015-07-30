#!/bin/bash -e

# Remember where we started
START_DIR="$(pwd)"

# ------
# ------ CMake Mario
# ------
mkdir -p cmake-build/macosx-workstation/ && cd cmake-build/macosx-workstation/
cmake -G Xcode ../../src/cpp

# ------
# ------ Build unit test binaries
# ------
xcodebuild -target test_buffer -configuration Debug

# ------
# ------ Run unit tests
# ------
ctest -C Debug -VV

cd $START_DIR

