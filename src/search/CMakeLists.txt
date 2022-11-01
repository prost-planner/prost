cmake_minimum_required(VERSION 2.8.12)

if(NOT PROST_MAIN_CMAKELISTS_READ)
  message(
    FATAL_ERROR
    "Run cmake on the CMakeLists.txt in the 'src' directory, "
    "not the one in 'src/search'. Please delete CMakeCache.txt "
    "and CMakeFiles/ from the current directory and restart cmake.")
endif()

## == Project ==
project(prost)
prost_set_compiler_flags()
prost_set_linker_flags()

## == BDD ==
find_package(BDD REQUIRED)

## == Includes ==
include_directories("logical_expressions_includes")
include_directories("utils")
include_directories(${BDD_INCLUDE_DIRS})

## == Source Files ==
set(SEARCH_SOURCES
    action_selection.cc
    backup_function.cc
    depth_first_search.cc
    evaluatables.cc
    initializer.cc
    ipc_client.cc
    iterative_deepening_search.cc
    logical_expressions.cc
    minimal_lookahead_search.cc
    outcome_selection.cc
    parser.cc
    probability_distribution.cc
    prost_planner.cc
    random_walk.cc
    recommendation_function.cc
    search_engine.cc
    states.cc
    thts.cc
    uniform_evaluation_search.cc
    utils/base64.cc
    utils/hash.cc
    utils/logger.cc
    utils/math_utils.cc
    utils/random.cc
    utils/stopwatch.cc
    utils/string_utils.cc
    utils/strxml.cc
    utils/system_utils.cc
)

## == Doctest ==
set(SEARCH_TEST_SOURCES
    ../doctest/doctest.h
    tests/evaluate_test.cc
    tests/probability_distribution_test.cc
)

# add unit test files in debug build
IF(CMAKE_BUILD_TYPE MATCHES Debug)
    set(SEARCH_SOURCES ${SEARCH_SOURCES} ${SEARCH_TEST_SOURCES})
ENDIF(CMAKE_BUILD_TYPE MATCHES Debug)

## == Add Executable ==
add_executable(search ${SEARCH_SOURCES} main.cc)

## == Link ==
target_link_libraries(search ${BDD_LIBRARIES})
