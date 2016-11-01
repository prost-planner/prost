cmake_minimum_required(VERSION 2.8.0)
project(rddl-parser)

include_directories("logical_expressions_includes")
include_directories("utils")

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

set(GPP_COMPILE_FLAGS "-g -Wall -W -Wno-sign-compare -Wno-deprecated -ansi -pedantic -Werror -std=c++0x") #-Wconversion"
set(GPP_LINK_FLAGS "-g")

set(FlexInput "lexer.l") #${CMAKE_CURRENT_BINARY_DIR}/parser.tab.hh")
set(BisonInput "parser.ypp")

find_package(BISON)
find_package(FLEX)

BISON_TARGET(parser ${BisonInput} ${CMAKE_CURRENT_BINARY_DIR}/parser.tab.cc COMPILE_FLAGS "-d" )
FLEX_TARGET(scanner ${FlexInput} ${CMAKE_CURRENT_BINARY_DIR}/lexer.cc)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
#include_directories(${CMAKE_CURRENT_BINARY_DIR})

ADD_FLEX_BISON_DEPENDENCY(scanner parser)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GPP_COMPILE_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${GPP_LINK_FLAGS}")

add_executable(rddl-parser ${RDDL_PARSER_SOURCES} ${FLEX_scanner_OUTPUTS} ${BISON_parser_OUTPUTS})

target_link_libraries(rddl-parser ${FLEX_LIBRARIES} ${BISON_LIBRARIES})