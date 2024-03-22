cmake_minimum_required(VERSION 3.22)
project(blocked_skiplist CXX)

add_compile_options(-O3)

set(CMAKE_CXX_STANDARD 20)

add_library(blocked_skiplist STATIC blocked_skiplist.hpp blocked_skiplist_node.hpp
        blocked_skiplist.cpp)

add_executable(test_blocked_skiplist test/test.cpp)
target_link_libraries(test_blocked_skiplist blocked_skiplist)