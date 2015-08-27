
cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

project(Mario)

find_package(PCAP)

# --------------------------------- Core -------------------------------

# -----
# ----- Mario Core
# -----
include_directories(protocols)
include_directories(core)

add_subdirectory(protocols)
add_subdirectory(core)

# --------------------------------- Libraries -------------------------------

# -----
# ----- Mario Client-only library
# -----
add_library(mario_client_only sys.cpp)

# --------------------------------- Platform -------------------------------
#add_library(core platform/darwin/darwin_host.cpp platform/posix/posix_host.cpp)
include_directories(platform/posix)
include_directories(platform/darwin)

# ----------------------------- Utilities/Binaries ---------------------------
add_executable(mario
                    platform/stdio/mario_cli.cpp
                    platform/darwin/darwin_host.cpp
                    platform/posix/posix_host.cpp
                    )
target_link_libraries(mario mario_client_only mwpcore mwpcoreprotos)

# --------------------------------- Unit Tests -------------------------------

# -----
# ----- Unit tests for the client-only classes
# -----
add_executable(test_client_only
                  test/test.cpp
                  test/test_buffer.cpp
                  test/test_mdns.cpp
                  test/test_utils.cpp
                  platform/darwin/darwin_host.cpp
                  platform/posix/posix_host.cpp
                  )

target_link_libraries(test_client_only mario_client_only mwpcore mwpcoreprotos pcap)

enable_testing(true)
add_test(NAME "Mario_client_only_test" COMMAND test_client_only)

