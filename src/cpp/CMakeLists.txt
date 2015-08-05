
cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

project(Mario)

# -----
# ----- Mario Core
# -----
include_directories(core)
add_subdirectory(core)

include_directories(protocols)
add_subdirectory(protocols)

# -----
# ----- Mario Client-only library
# -----
add_library(mario_client_only sys.cpp)

# -----
# ----- Unit tests for the client-only classes
# -----
add_executable(test_client_only
                  test/test.cpp
                  test/test_buffer.cpp
                  test/test_mdns.cpp
                  )

target_link_libraries(test_client_only mario_client_only mwpcore)

enable_testing(true)
add_test(NAME "Mario_client_only_test" COMMAND test_client_only)

