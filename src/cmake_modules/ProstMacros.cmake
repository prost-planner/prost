macro(prost_set_compiler_flags)
# Note: on CMake >= 3.0 the compiler ID of Apple-provided clang is AppleClang.
# If we change the required CMake version from 2.8.3 to 3.0 or greater,
# we have to fix this.

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -W -Wno-sign-compare")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated -ansi")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic -Werror -std=c++0x") #-Wconversion"  

# Configuration-specific flags
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -fomit-frame-pointer")
set(CMAKE_CXX_FLAGS_DEBUG "-fprofile-arcs -ftest-coverage -lgcov")
set(CMAKE_CXX_FLAGS_TEST "-I../test -DGTEST_HAS_PTHREAD=0")
endmacro()

macro(prost_set_linker_flags_search)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -g -lbdd")
  set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS} -O3")
  set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS} -O3 -lgcov")
endmacro()

macro(prost_set_linker_flags_rddl_parser)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -g")
  set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS} -O3")
  set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS} -O3 -lgcov")
endmacro()