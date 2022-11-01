cmake_minimum_required(VERSION 2.8.12)

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
prost_set_linker_flags()

## == Flex / Bison ==
set(FlexInput "lexer.l")
set(BisonInput "parser.ypp")

find_package(BISON)
find_package(FLEX)

BISON_TARGET(parser ${BisonInput} ${CMAKE_CURRENT_BINARY_DIR}/parser.tab.cc COMPILE_FLAGS "-d" )
FLEX_TARGET(scanner ${FlexInput} ${CMAKE_CURRENT_BINARY_DIR}/lexer.cc)
ADD_FLEX_BISON_DEPENDENCY(scanner parser)

## == Z3 ==
cmake_policy(SET CMP0074 NEW)
find_package(Z3 REQUIRED)

## == Includes ==
include_directories("logical_expressions_includes")
include_directories("utils")
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${Z3_INCLUDE_DIRS})

## == Source Files ==
set(RDDL_PARSER_SOURCES
    csp.cc
    determinize/determinize.cc
    evaluatables.cc
    fdr/fdr_generation.cc
    fdr/mutex_detection.cc
    hashing/hash_keys.cc
    instantiator.cc
    logical_expressions.cc
    precomputer.cc
    probability_distribution.cc
    rddl.cc
    reachability_analysis.cc
    simplifier.cc
    states.cc
    task_analyzer.cc
    utils/math.cc
    utils/system.cc
    utils/timer.cc
)

## == Doctest ==
set(RDDL_PARSER_TEST_SOURCES
    ../doctest/doctest.h
    tests/csp_test.cc
    tests/determinize_test.cc
    tests/fdr_generation_test.cc
    tests/hash_keys_test.cc
    tests/logical_expressions_test.cc
    tests/mutex_detection_test.cc
    tests/probability_distribution_test.cc
    tests/reachability_analysis_test.cc
)

# add unit test files in debug build
IF(CMAKE_BUILD_TYPE MATCHES Debug)
    set(RDDL_PARSER_SOURCES ${RDDL_PARSER_SOURCES} ${RDDL_PARSER_TEST_SOURCES})
ENDIF(CMAKE_BUILD_TYPE MATCHES Debug)

## == Add Executable ==
add_executable(rddl-parser ${RDDL_PARSER_SOURCES} ${FLEX_scanner_OUTPUTS} ${BISON_parser_OUTPUTS})

## == Link ==
target_link_libraries(rddl-parser ${Z3_LIBRARIES})
