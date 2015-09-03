#!/bin/bash -e

die()
{
  echo "$@"
  exit 1
}

show_usage()
{
  echo "Usage: mk [--flags]"
  echo ""
  echo "  --full  --  Build all platforms for Debug"
  echo "  --all   --  Build everything"

  exit 2
}

test -z "$WORKSPACE" && WORKSPACE="$(pwd)/.."

for arg in "$@"; do
  case $arg in

    --full)
      full="1"
      shift;;

    --all)
      all="1"
      shift;;

    --clean)
      clean="1"
      shift;;

  esac
done

# -----
# ----- Clean the build tree
# -----
if [ -n "$clean" ]; then
  START_CLEAN_DIR=$(pwd)
  test -d cmake-build && rm -rf cmake-build
  test -d deliveries && rm -rf deliveries

  cd src/java && ndk-build clean

  cd $START_CLEAN_DIR
  exit 0
fi

# Remember where we started
START_DIR="$(pwd)"

# First things first -- build and pass the unit tests, then worry about building the full complement of
# configurations.

echo ""
echo ""
echo "===================================================================================="
echo "====================================== MAC OSX - Debug ============================="
echo "===================================================================================="
echo ""
# ------
# ------ CMake Mario for OS-X
# ------
mkdir -p cmake-build/macosx-workstation/ && cd cmake-build/macosx-workstation/
cmake -G Xcode ../../src/cpp

echo ""
echo ""
echo "===================================================================================="
echo "====================================== Tests ======================================="
echo "===================================================================================="
echo ""

# ------
# ------ Build unit test binaries
# ------
xcodebuild -target test_client_only -configuration Debug

# ------
# ------ Run unit tests
# ------

#ctest -C Debug -VV
./Debug/test_client_only

echo ""
echo ""
echo "===================================================================================="
echo "====================================== MAC OSX - Debug - Utils ====================="
echo "===================================================================================="
echo ""
# ------
# ------ Build Utilities
# ------
xcodebuild -target mario -configuration Debug

cd $START_DIR

# Stop, if we are only doing a quick development build
if [ -z "$full" -a -z "$all" ]; then
  exit 0
fi

echo ""
echo ""
echo "===================================================================================="
echo "====================================== IOS - Debug ================================="
echo "===================================================================================="
echo ""
# ----- Build iOS debug -----
mkdir -p cmake-build/ios && cd cmake-build/ios
cmake -G Xcode ../../src/cpp -DCMAKE_TOOLCHAIN_FILE=${WORKSPACE}/client/toolchain/iOS.cmake -DIOS_PLATFORM="SIMULATOR"
#xcodebuild -target mario_client_only -configuration Debug
cd $START_DIR

echo ""
echo ""
echo "===================================================================================="
echo "====================================== Android - Debug ============================="
echo "===================================================================================="
echo ""
# ----- Build Android debug -----
mkdir -p cmake-build/android && cd cmake-build/android
cmake -G "Unix Makefiles" ../../src/cpp -DCMAKE_TOOLCHAIN_FILE=${WORKSPACE}/client/toolchain/android.cmake -DCMAKE_BUILD_TYPE=Debug -DANDROID_STL=stlport_static
#make mario_client_only

cd $START_DIR/src/java
ndk-build

# Finish in the dir that we started
cd $START_DIR

