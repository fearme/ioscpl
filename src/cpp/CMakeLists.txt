
cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

project(Mario)

include_directories(core)
add_subdirectory(core)

add_library(mario_client_only sys.cpp)

add_executable(test_buffer test/test_buffer.cpp)
target_link_libraries(test_buffer mario_client_only)

enable_testing(true)
add_test(NAME "Mario_test_buffer" COMMAND test_buffer)

