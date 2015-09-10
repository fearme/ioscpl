
cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

project(Mario)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "../../toolchain")
find_package(RAGEL 6.6 REQUIRED)

# --------------------------------- Core -------------------------------

# -----
# ----- Mario Core
# -----
include_directories(protocols)
include_directories(parsers)
include_directories(core)

add_subdirectory(protocols)
add_subdirectory(parsers)
add_subdirectory(core)

# --------------------------------- Platform -------------------------------

if (IOS)
  set(MARIO_PLATFORM   "ios")
  find_host_package(RAGEL 6.6 REQUIRED)

  include_directories(platform/posix)
  include_directories(platform/darwin)

  set(PLATFORM_SRC ${PLATFORM_SRC}
      platform/android/android_host.cpp
      platform/posix/posix_host.cpp
      )

  set(MARIO_SHAREDNESS "")
  set(MARIO_CLI        false)
  set(MARIO_APP        true)
  set(MARIO_UNIT_TESTS false)

elseif (APPLE)
  set(MARIO_PLATFORM   "mac_osx")

  include_directories(platform/posix)
  include_directories(platform/darwin)

  set(PLATFORM_SRC ${PLATFORM_SRC}
      platform/darwin/darwin_host.cpp
      platform/posix/posix_host.cpp
      )

  set(MARIO_SHAREDNESS "")
  set(MARIO_CLI        true)
  set(MARIO_APP        false)
  set(MARIO_UNIT_TESTS true)

else()
  set(MARIO_PLATFORM     "android")
  find_host_package(RAGEL 6.6 REQUIRED)

  include_directories(platform/posix)
  include_directories(platform/android)

  set(PLATFORM_SRC ${PLATFORM_SRC}
      platform/android/android_host.cpp
      platform/posix/posix_host.cpp
      )

  set(MARIO_SHAREDNESS   "SHARED")
  set(MARIO_CLI          false)
  set(MARIO_APP          true)
  set(MARIO_UNIT_TESTS   false)

endif()

# --------------------------------- Libraries -------------------------------

# -----
# ----- Mario Client-only library
# -----
set(MARIO_CLIENT_ONLY ${MARIO_CLIENT_ONLY} mwpcore mwpcoreprotos mwpparsers)

# ----------------------------- Utilities/Binaries ---------------------------
if (${MARIO_CLI})
  add_executable(mario
                      platform/stdio/mario_cli.cpp
                      ${PLATFORM_SRC}
                      )

  target_link_libraries(mario ${MARIO_CLIENT_ONLY})
endif()

# --------------------------------- Unit Tests -------------------------------

# -----
# ----- Unit tests for the client-only classes
# -----
if (${MARIO_UNIT_TESTS})

  find_package(PCAP REQUIRED)

  add_executable(test_client_only
                    test/test.cpp
                    test/test_buffer.cpp
                    test/test_mdns.cpp
                    test/test_utils.cpp
                    test/test_json.cpp
                    platform/darwin/darwin_host.cpp
                    platform/posix/posix_host.cpp
                    )

  target_link_libraries(test_client_only ${MARIO_CLIENT_ONLY} pcap)

  enable_testing(true)
  add_test(NAME "Mario_client_only_test" COMMAND test_client_only)
endif()

