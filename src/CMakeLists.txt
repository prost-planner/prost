cmake_minimum_required(VERSION 2.8.0)

# Respect the PATH environment variable when searching for compilers.
find_program(CMAKE_C_COMPILER NAMES $ENV{CC} gcc PATHS ENV PATH NO_DEFAULT_PATH)
find_program(CMAKE_CXX_COMPILER NAMES $ENV{CXX} g++ PATHS ENV PATH NO_DEFAULT_PATH)

# Path containing custom CMake modules
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules)
include(ProstMacros)

project(prost)

set(PROST_MAIN_CMAKELISTS_READ TRUE)

add_subdirectory(rddl_parser)
add_subdirectory(search)
