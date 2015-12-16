#
#
# This is the root CMakeLists.txt file for Mario-client.
#
# It uses the following ENV-vars, if available.
#   $BUILD_NUMBER
#                     -- The Build number from Jenkins (or '1' if building locally.)
#   $USER
#                     -- The Build user
#   $MARIO_BRANCH
#                     -- The git branch name being built
#
#

cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

project("mario_client")

# ----------------------------------------------------------------------------------------------------
# Inform CMAKE of various paths and things
#
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../../toolchain")

# ----------------------------------------------------------------------------------------------------
# Various options
#
# build_tests (ON)
#
option(build_tests "Build and run the tests." ON)

# ====================================================================================================
# Figure out our build variables
# ====================================================================================================

# ----------------------------------------------------------------------------------------------------
# Compute:
#   ${BUILD_NUMBER}           -- The Jenkins build number
#   ${BUILD_USER}             -- The username of the build user ("Build" for jenkins builds, $USER otherwise)
#
set(BUILD_NUMBER  $ENV{BUILD_NUMBER})
set(BUILD_USER    $ENV{USER})

# ----------------------------------------------------------------------------------------------------
# Compute:
#   ${MARIO_MAJOR_VERSION}    -- Like "1.1"
#   ${MARIO_VERSION}          -- Like "1.1.57"
#   ${MARIO_DATA_FORMAT}
#       11 -- the version for Mario 1.1
#
set(MARIO_MAJOR_VERSION   "1.1")
set(MARIO_VERSION         "1.1.${BUILD_NUMBER}")
set(MARIO_DATA_FORMAT        11)

# ----------------------------------------------------------------------------------------------------
# Compute:
#   ${MARIO_GIT_SHA1}         -- The long SHA1 of the commit that we are building
#   ${MARIO_GIT_SHA1_SHORT}   -- The short SHA1 of the commit that we are building
#
execute_process(COMMAND git rev-parse HEAD                OUTPUT_VARIABLE MARIO_GIT_SHA1          OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND git rev-parse --short HEAD        OUTPUT_VARIABLE MARIO_GIT_SHA1_SHORT    OUTPUT_STRIP_TRAILING_WHITESPACE)

# ----------------------------------------------------------------------------------------------------
# Compute:
#   ${MARIO_GIT_BRANCH_NAME}  -- The branch name we are building
#
execute_process(COMMAND git rev-parse --abbrev-ref HEAD   OUTPUT_VARIABLE MARIO_GIT_BRANCH_NAME   OUTPUT_STRIP_TRAILING_WHITESPACE)

# If you build on a 'detached' branch, you get "HEAD", so re-compute, if possible
if (${MARIO_GIT_BRANCH_NAME} STREQUAL "HEAD")
  set(MARIO_GIT_BRANCH_NAME    "$ENV{MARIO_BRANCH}")
endif()

# ----------------------------------------------------------------------------------------------------
# Compute:
#   ${MARIO_STACK_NAME}     -- The name of the stack that the client will attach to by default
#   ${MARIO_DOMAIN_NAME}    -- The domain name of the stack that the client will attach to by default
#   ${MARIO_IS_DEBUG}       -- Is this a 'debug' build?
#   ${MARIO_VERBOSITY}      -- What is the default verbosity of the output logging
#
if (${MARIO_GIT_BRANCH_NAME} STREQUAL "prod")

  set(MARIO_STACK_NAME "prod")
  set(MARIO_DOMAIN_NAME "mobilewebprint.net")
  set(MARIO_IS_DEBUG    false)
  set(MARIO_VERBOSITY   "verbose")

else()

  set(MARIO_DOMAIN_NAME "mobiledevprint.net")
  set(MARIO_IS_DEBUG    true)
  set(MARIO_VERBOSITY   "vverbose")

  if (${MARIO_GIT_BRANCH_NAME} STREQUAL "qa")
    set(MARIO_STACK_NAME "qa")
  elseif (${MARIO_GIT_BRANCH_NAME} STREQUAL "stg")
    set(MARIO_STACK_NAME "ext")
  else()
    set(MARIO_STACK_NAME "dev")
    set(MARIO_VERBOSITY   "vvverbose")
  endif()
endif()

# ----------------------------------------------------------------------------------------------------
# Compute:
#   ${MARIO_DEFAULT_FQDN}     -- The fully-qualified-domain-name that we are attaching to.
#
set(MARIO_DEFAULT_FQDN  "${MARIO_STACK_NAME}.${MARIO_DOMAIN_NAME}")

# ----------------------------------------------------------------------------------------------------
# Compute:
#   ${MARIO_CURL_AGENT}       -- The User-Agent for mario-client requests
#
set(MARIO_CURL_AGENT "Mario")
if(NOT ${BUILD_USER} STREQUAL "build")
  if(${BUILD_USER} STREQUAL "sparksb")
    set(MARIO_CURL_AGENT "${MARIO_CURL_AGENT}-s")
  else()
    set(MARIO_CURL_AGENT "${MARIO_CURL_AGENT}-${BUILD_USER}")
  endif()
endif()

set(MARIO_CURL_AGENT "${MARIO_CURL_AGENT}-${MARIO_PLATFORM}-${MARIO_GIT_BRANCH_NAME} ${MARIO_VERSION} (${MARIO_GIT_SHA1_SHORT}) libcurl")

# ----------------------------------------------------------------------------------------------------
# Determine which platform we are building for.
#
#   ${MARIO_PLATFORM}
#                       -- 'ios'      == Building for iOS
#                       -- 'macosx'   == Building for Mac
#                       -- 'android'  == Building for Android
#
if(IOS)
  set(MARIO_PLATFORM "ios")
elseif (APPLE)
  set(MARIO_PLATFORM "macosx")
elseif (ANDROID_STANDALONE_TOOLCHAIN_SEARCH_PATH)
  set(MARIO_PLATFORM "android")
else()
  message(FATAL_ERROR "Unknown platform.  Exiting.")
endif()

# ----------------------------------------------------------------------------------------------------
# Turn testing on -- not when cross-compiling
#
if (${MARIO_PLATFORM} STREQUAL "macosx")

  if (build_tests)
    enable_testing()

    find_package(GTEST)
    #message(STATUS ${GTEST_FOUND})
    if (GTEST_FOUND)
      message(STATUS "Found gtest at: ${GTEST_INCLUDE_DIRS}")
      message(STATUS "Found gtest at: ${GTEST_LIBRARIES}")

      include_directories(${GTEST_INCLUDE_DIRS})
    else()
      message(STATUS "-----------------------!!!!!!!!!!!!!!! build_tests enabled, but no GTest found")
    endif ()

    find_package(GMock)
    #message(STATUS ${GMOCK_FOUND})
    if (GMOCK_FOUND)
      message(STATUS "Found gmock at: ${GMOCK_INCLUDE_DIRS}")
      message(STATUS "Found gmock at: ${GMOCK_LIBRARIES}")

      include_directories(${GMOCK_INCLUDE_DIRS})
    else()
      message(STATUS "-----------------------!!!!!!!!!!!!!!! build_tests enabled, but no GMock found")
    endif ()
  endif()

endif()
#message(FATAL_ERROR "Stopping")

# ----------------------------------------------------------------------------------------------------
# Add the directories for the modules:
#
#     mario_root/client/src/cpp/core          -- The core of the mario-client implementation
#     mario_root/client/src/cpp/cpl           -- The controlled-print-library extensions
#     mario_root/client/src/cpp/aprotocols    -- The 'raw' networking protocols (the 'a' is just to put them at the top of a sorted list)
#     mario_root/client/src/cpp/objc          -- The Objective-C interoperability layer
#
# Also, add includes:
#     mario_root/client/src/cpp/curl          -- The cUrl header libraries
#
# Note: Adding ${PROJECT_BINARY_DIR} versions will also add the CMake-generated files to be found (*.h.in)
#
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

include_directories(core)
include_directories(curl)
include_directories(cpl)
include_directories(aprotocols)

add_subdirectory(core)
include_directories("${PROJECT_BINARY_DIR}/core")

add_subdirectory(cpl)
include_directories("${PROJECT_BINARY_DIR}/cpl")

add_subdirectory(aprotocols)
include_directories("${PROJECT_BINARY_DIR}/aprotocols")

add_subdirectory(objc)
include_directories("${PROJECT_BINARY_DIR}/objc")

# ====================================================================================================
# Set some key mario-specific variables
#
#   MARIO_CURL_CPP      -- The CPP file to use for controlling curl
#   MARIO_PLATFORM_DIR  -- The system-specific platform code
#   MARIO_PLATFORM_LOG  -- A platform-specific implementation of the log_d type functions
#   MARIO_APP           -- Is this for an app?  (Something that runs on a mobile, as opposed to running on a desktop.)
#   MARIO_APP_SHARED    -- If it is for an app, do we want static libs, or shared?

# Use the NULL curl implementation, if another better one does not show up
set(MARIO_CURL_CPP "core/mwp_curl_null.cpp")

# What platform are we on?
if(${MARIO_PLATFORM} STREQUAL "ios")
  set(MARIO_PLATFORM_DIR platform/ios)
  set(MARIO_PLATFORM_LOG ${MARIO_PLATFORM_DIR})
  set(MARIO_APP True)
  set(MARIO_APP_SHARED False)
  set(MARIO_CURL_CPP "core/mwp_curl.cpp")
elseif (${MARIO_PLATFORM} STREQUAL "macosx")
  set(MARIO_PLATFORM_DIR platform/macosx)
  set(MARIO_PLATFORM_LOG platform/darwin)
  set(MARIO_APP False)
  set(MARIO_CURL_CPP "core/mwp_curl.cpp")
  set(CMAKE_MACOSX_RPATH 1)
elseif (${MARIO_PLATFORM} STREQUAL "android")
  set(MARIO_PLATFORM_DIR platform/android)
  set(MARIO_PLATFORM_LOG ${MARIO_PLATFORM_DIR})
  set(MARIO_APP True)
  set(MARIO_APP_SHARED True)
  set(MARIO_CURL_CPP "core/mwp_curl.cpp")
endif()

# ====================================================================================================
# Build up the platform-specific list of sources
#
#   MARIO_PLATFORM_SOURCES      -- List of source files that are specific for the platform
#
set(MARIO_PLATFORM_SOURCES platform/base/mario_sys.cpp)
set(MARIO_PLATFORM_SOURCES ${MARIO_PLATFORM_SOURCES} ${MARIO_PLATFORM_DIR}/mario_sys.cpp)

if(${MARIO_PLATFORM} STREQUAL "ios")
  set(MARIO_PLATFORM_SOURCES ${MARIO_PLATFORM_SOURCES} platform/darwin/mario_sys.cpp)
  set(MARIO_PLATFORM_SOURCES ${MARIO_PLATFORM_SOURCES} platform/ios/platform_name.cpp)
elseif (${MARIO_PLATFORM} STREQUAL "macosx")
  set(MARIO_PLATFORM_SOURCES ${MARIO_PLATFORM_SOURCES} platform/darwin/mario_sys.cpp)
  set(MARIO_PLATFORM_SOURCES ${MARIO_PLATFORM_SOURCES} platform/darwin/platform_name.cpp)
endif()

# ====================================================================================================
# Build the main library and/or the cli utility
# ====================================================================================================

# ----------------------------------------------------------------------------------------------------
# Build iOS
#
if(${MARIO_PLATFORM} STREQUAL "ios")

  add_library(mwp
              ${MARIO_PLATFORM_SOURCES}
              ${MARIO_CURL_CPP}
              ${MARIO_PLATFORM_LOG}/mario_logging.cpp)

  target_link_libraries(mwp mwpcore mwpcoreprotos cpl mwpobjcinterop)

# ----------------------------------------------------------------------------------------------------
# Build Android
#
elseif (${MARIO_PLATFORM} STREQUAL "android")

  find_package(ZLIB REQUIRED)
  if (ZLIB_FOUND)
    include_directories(${ZLIB_INCLUDE_DIRS})

    add_library(mwp SHARED
                ${MARIO_PLATFORM_SOURCES}
                ${MARIO_CURL_CPP}
                ${MARIO_PLATFORM_LOG}/mario_logging.cpp)

    target_link_libraries(mwp mwpcore mwpcoreprotos cpl log
                ${CMAKE_CURRENT_SOURCE_DIR}/platform/android/libs/${ANDROID_ABI}/libcurl.a
                ${ZLIB_LIBRARIES})
  endif()

# ----------------------------------------------------------------------------------------------------
# Build Mac
#
elseif (${MARIO_PLATFORM} STREQUAL "macosx")

  find_package(CURL REQUIRED)

  # "mariocli" is a command-line tool to drive functionality during development
  add_executable(mwpcli
                 ${MARIO_PLATFORM_SOURCES}
                 ${MARIO_CURL_CPP}
                 platform/stdio_out_err/mario_cli.cpp)

  #target_link_libraries(mwpcli mwpcore mwpcoreprotos cpl mwpobjcinterop)
  target_link_libraries(mwpcli mwpcore mwpcoreprotos cpl mwpobjcinterop
                          ${CURL_LIBRARIES}
                          "-framework CoreFoundation"
                          "-framework IOKit"
                          "-framework AppKit"
                          "-framework SystemConfiguration")

  if (build_tests)
    if (GTEST_FOUND)
      add_executable(_0_test_gtest "tests/setup/test-gtest.cpp")
      target_link_libraries(_0_test_gtest ${GTEST_LIBRARIES} ${GTEST_MAIN_LIBRARIES})
      add_test(_0_test_gtest, _0_test_gtest)

      add_executable(_0_test_gmock "tests/setup/test-gmock.cpp")
      target_link_libraries(_0_test_gmock ${GMOCK_LIBRARIES} ${GMOCK_MAIN_LIBRARIES})
      add_test(_0_test_gmock, _0_test_gmock)
    else()
      message(STATUS "-----------------------!!!!!!!!!!!!!!! build_tests enabled, but no GTest found")
    endif()

  endif()

endif()

#add_test(marioTest mwp)

