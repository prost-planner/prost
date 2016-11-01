cmake_minimum_required(VERSION 2.8.0)
project(prost)

include_directories("logical_expressions_includes")
include_directories("utils")

set(SEARCH_SOURCES
  main
  action_selection
  backup_function
  depth_first_search
  evaluatables
  initializer
  ippc_client
  iterative_deepening_search
  logical_expressions
  minimal_lookahead_search
  outcome_selection
  parser
  probability_distribution
  prost_planner
  random_walk
  recommendation_function
  search_engine
  states
  thts
  uniform_evaluation_search
  utils/math_utils
  utils/random
  utils/stopwatch
  utils/string_utils
  utils/strxml
  utils/system_utils
 )

# Storing flags in variables
set(GPP_COMPILE_FLAGS "-g -Wall -W -Wno-sign-compare -Wno-deprecated -ansi -pedantic -Werror -std=c++0x") #-Wconversion"
set(GPP_LINK_FLAGS "-g -lbdd")

# Setting flags.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GPP_COMPILE_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${GPP_LINK_FLAGS}")

add_executable(search ${SEARCH_SOURCES})

# Linking BDD
target_link_libraries(search bdd)

