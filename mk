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

    --dev)
      dev="1"
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

  cd src/java && ndk-build clean && ant clean

  cd $START_CLEAN_DIR
  test -d src/java/libs && rm -rf src/java/libs
  test -d src/java/obj && rm -rf src/java/obj

  cd $START_CLEAN_DIR

  echo ""
  echo "===================================================================================="
  echo "The build is clean.  Use 'mk --dev' or 'mk' or 'mk --full' or 'mk --all'"
  echo "===================================================================================="
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

# ----------------------------------------------------
# Stop, if we are doing a development-only build.
# ---------------------------------------------------
if [ -n "$dev" ]; then
  echo "=================================================================================================="
  echo "Unit tests built and passing.  Use 'mk' or 'mk --full' to build debug versions of all platforms."
  echo "=================================================================================================="
  exit 0
fi

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

# ----------------------------------------------------
# Stop, if we are only doing a quick development build
# ---------------------------------------------------
if [ -z "$full" -a -z "$all" ]; then
  echo "===================================================================================="
  echo "Unit tests built and passing.  Use --full to build debug versions of all platforms."
  echo "===================================================================================="
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
ant dist

cd $START_DIR
${START_DIR}/scripts/publish-android --debug

# ----------------------------------------------------
# Stop, if we are only doing a full build
# ---------------------------------------------------
if [ -z "$all" ]; then
  echo "==============================================================================================================="
  echo "Debug versions of all platforms have been built.  Use --all to build everything (debug and release versions.)"
  echo "==============================================================================================================="
  exit 0
fi

# Finish in the dir that we started
cd $START_DIR

