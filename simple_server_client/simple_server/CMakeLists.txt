cmake_minimum_required(VERSION 3.1)
project(simple_server C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add debug symbols
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")

# Set include directories
include_directories(../_build_install/install/usr/include)

# Set library directories
link_directories(../_build_install/install/usr/lib)

# Add executable with source files
add_executable(simple_server simple_server.cpp)

# Link libraries
target_link_libraries(simple_server fdbus pthread)


