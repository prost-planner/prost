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
find_package(Z3 REQUIRED)

## == Includes ==
include_directories("logical_expressions_includes")
include_directories("utils")
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${Z3_INCLUDE_DIRS})

## == Source Files ==
set(RDDL_PARSER_SOURCES
    csp
    determinizer
    evaluatables
    fdr_generation
    hash_key_generator
    instantiator
    logical_expressions
    mutex_detection
    precomputer
    probability_distribution
    rddl
    reachability_analysis
    simplifier
    states
    task_analyzer
    utils/math_utils
    utils/string_utils
    utils/system_utils
    utils/timer
)

## == Doctest ==
set(RDDL_PARSER_TEST_SOURCES
    ../doctest/doctest
    tests/fdr_generation_test
    tests/logical_expressions_test
    tests/mutex_detection_test
    tests/probability_distribution_test
    tests/csp_test
    tests/reachability_analysis_test
)

# add unit test files in debug build
IF(CMAKE_BUILD_TYPE MATCHES Debug)
    set(RDDL_PARSER_SOURCES ${RDDL_PARSER_SOURCES} ${RDDL_PARSER_TEST_SOURCES})
ENDIF(CMAKE_BUILD_TYPE MATCHES Debug)

## == Add Executable ==
add_executable(rddl-parser ${RDDL_PARSER_SOURCES} ${FLEX_scanner_OUTPUTS} ${BISON_parser_OUTPUTS})

## == Link ==
target_link_libraries(rddl-parser ${Z3_LIBRARIES})