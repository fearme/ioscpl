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
xcodebuild -target test_client_only -configuration Debug

# ------
# ------ Run unit tests
# ------

echo ""
echo ""
echo "===================================================================================="
echo "====================================== Tests ======================================="
echo "===================================================================================="
echo ""
#ctest -C Debug -VV
./Debug/test_client_only

cd $START_DIR

