
cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
#set(CMAKE_USE_RELATIVE_PATHS ON FORCE)

project("mario_client")

set(BUILD_NUMBER  $ENV{BUILD_NUMBER})
set(BUILD_USER    $ENV{USER})

# TODO: move this to the right place (in an 'if' that knows we are building
# for MacOSX)
set(CMAKE_MACOSX_RPATH 1)

if(IOS)
  set(MARIO_PLATFORM "ios")
elseif (APPLE)
  set(MARIO_PLATFORM "macosx")
elseif (ANDROID_STANDALONE_TOOLCHAIN_SEARCH_PATH)
  set(MARIO_PLATFORM "android")
else()
  message(FATAL_ERROR "Unknown platform.  Exiting.")
endif()

#enable_testing()

# Sources and includes are:
#   1. In a sub-dir of this file -- ./core/
#   2. In this file's dir
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# ./core/CMakeLists.txt defines the "mwpcore" static library
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

# Use the NULL curl implementation, if another better one does not show up
set(MARIO_CURL_CPP "core/mwp_curl_null.cpp")

# Set some key mario-specific variables
#
#   MARIO_BASE_CPP      -- The base implementation of the system-specific platform code (like POSIX works for Android and Darwin)
#   MARIO_PLATFORM_DIR  -- The system-specific platform code
#   MARIO_PLATFORM_LOG  -- A platform-specific implementation of the log_d type functions
#   MARIO_APP           -- Is this for an app?  (Something that runs on a mobile, as opposed to running on a desktop.)
#   MARIO_APP_SHARED    -- If it is for an app, do we want static libs, or shared?
#   MARIO_CURL_CPP      -- The CPP file to use for controlling curl

set(MARIO_PLATFORM_SOURCES platform/base/mario_sys.cpp)

# What platform are we on?
if(${MARIO_PLATFORM} STREQUAL "ios")
  set(MARIO_PLATFORM_DIR platform/darwin)
  set(MARIO_PLATFORM_LOG platform/ios)
  set(MARIO_APP True)
  set(MARIO_APP_SHARED False)
  set(MARIO_CURL_CPP "core/mwp_curl.cpp")
elseif (${MARIO_PLATFORM} STREQUAL "macosx")
  set(MARIO_PLATFORM_DIR platform/darwin)
  set(MARIO_PLATFORM_LOG ${MARIO_PLATFORM_DIR})
  set(MARIO_APP False)
elseif (${MARIO_PLATFORM} STREQUAL "android")
  set(MARIO_PLATFORM_DIR platform/android)
  set(MARIO_PLATFORM_LOG ${MARIO_PLATFORM_DIR})
  set(MARIO_APP True)
  set(MARIO_APP_SHARED True)
  set(MARIO_CURL_CPP "core/mwp_curl.cpp")
endif()

set(MARIO_PLATFORM_SOURCES ${MARIO_PLATFORM_SOURCES} ${MARIO_PLATFORM_DIR}/mario_sys.cpp)

if(${MARIO_PLATFORM} STREQUAL "ios")
  set(MARIO_PLATFORM_SOURCES ${MARIO_PLATFORM_SOURCES} platform/ios/platform_name.cpp)
elseif (${MARIO_PLATFORM} STREQUAL "macosx")
  set(MARIO_PLATFORM_SOURCES ${MARIO_PLATFORM_SOURCES} platform/darwin/platform_name.cpp)
endif()

# Build the main library
if(${MARIO_PLATFORM} STREQUAL "ios")

  add_library(mwp
              ${MARIO_PLATFORM_SOURCES}
              ${MARIO_CURL_CPP}
              ${MARIO_PLATFORM_LOG}/mario_logging.cpp)

  target_link_libraries(mwp mwpcore mwpcoreprotos cpl mwpobjcinterop)

elseif (${MARIO_PLATFORM} STREQUAL "macosx")

  # "mariocli" is a command-line tool to drive functionality during development
  add_executable(mwpcli
                 ${MARIO_PLATFORM_SOURCES}
                 ${MARIO_CURL_CPP}
                 platform/stdio_out_err/mario_cli.cpp)

  target_link_libraries(mwpcli mwpcore mwpcoreprotos)


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

endif()

#add_test(marioTest mwp)

