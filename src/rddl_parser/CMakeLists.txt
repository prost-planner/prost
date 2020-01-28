cmake_minimum_required(VERSION 2.8.0)

if(NOT PROST_MAIN_CMAKELISTS_READ)
  message(
    FATAL_ERROR
    "Run cmake on the CMakeLists.txt in the 'src' directory, "
    "not the one in 'src/rddl_parser'. Please delete CMakeCache.txt "
    "and CMakeFiles/ from the current directory and restart cmake.")
endif()

## == Project ==

project(rddl-parser)
prost_set_compiler_flags()
prost_set_linker_flags_rddl_parser()

## == Includes ==

include_directories("logical_expressions_includes")
include_directories("utils")
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

## == Source Files ==

set(RDDL_PARSER_SOURCES
  evaluatables
  instantiator
  logical_expressions
  preprocessor
  probability_distribution
  rddl
  states
  task_analyzer
  utils/math_utils
  utils/string_utils
  utils/system_utils
  utils/timer
 )

set(RDDL_PARSER_TESTS
  ../doctest/doctest
  tests/probability_distribution_test
)

## == Flex / Bison ==

set(FlexInput "lexer.l") #${CMAKE_CURRENT_BINARY_DIR}/parser.tab.hh")
set(BisonInput "parser.ypp")

find_package(BISON)
find_package(FLEX)

BISON_TARGET(parser ${BisonInput} ${CMAKE_CURRENT_BINARY_DIR}/parser.tab.cc COMPILE_FLAGS "-d" )
FLEX_TARGET(scanner ${FlexInput} ${CMAKE_CURRENT_BINARY_DIR}/lexer.cc)
ADD_FLEX_BISON_DEPENDENCY(scanner parser)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GPP_COMPILE_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${GPP_LINK_FLAGS}")

## == Add executable ==

# disable doctest in release build
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DDOCTEST_CONFIG_DISABLE")

# only enable unit tests in debug build
IF(CMAKE_BUILD_TYPE MATCHES Debug)
    add_executable(rddl-parser ${RDDL_PARSER_SOURCES} ${RDDL_PARSER_TESTS} ${FLEX_scanner_OUTPUTS} ${BISON_parser_OUTPUTS})
ELSE()
    add_executable(rddl-parser ${RDDL_PARSER_SOURCES} ${FLEX_scanner_OUTPUTS} ${BISON_parser_OUTPUTS})
ENDIF(CMAKE_BUILD_TYPE MATCHES Debug)

