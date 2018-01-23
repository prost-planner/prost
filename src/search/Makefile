HEADERS = parser.h \
	  prost_planner.h \
	  search_engine.h \
	  thts.h \
	  action_selection.h \
	  outcome_selection.h \
	  backup_function.h \
	  initializer.h \
	  recommendation_function.h \
	  depth_first_search.h \
	  iterative_deepening_search.h \
	  minimal_lookahead_search.h \
	  uniform_evaluation_search.h \
	  random_walk.h \
	  ippc_client.h \
	  states.h \
	  evaluatables.h \
	  logical_expressions.h \
	  probability_distribution.h \
	  utils/base64.h \
	  utils/strxml.h \
	  utils/stopwatch.h \
	  utils/string_utils.h \
	  utils/system_utils.h \
	  utils/math_utils.h \
	  utils/random.h \

SOURCES = main.cc $(HEADERS:%.h=%.cc)

SHELL = /bin/bash

TARGET  = prost

OBJECT_SUFFIX_RELEASE =
TARGET_SUFFIX_RELEASE =
OBJECT_SUFFIX_DEBUG   = .debug
TARGET_SUFFIX_DEBUG   = -debug

OBJECTS_RELEASE = $(SOURCES:%.cc=.obj/%$(OBJECT_SUFFIX_RELEASE).o)
TARGET_RELEASE  = $(TARGET)$(TARGET_SUFFIX_RELEASE)

OBJECTS_DEBUG   = $(SOURCES:%.cc=.obj/%$(OBJECT_SUFFIX_DEBUG).o)
TARGET_DEBUG    = $(TARGET)$(TARGET_SUFFIX_DEBUG)

CC     = g++
DEPEND = g++ -MM -std=c++0x

## Test related
GTEST_DIR = ../test/gtest
TEST_MAIN = ../test/testMain
TEST_BINARIES = $(basename $(wildcard ../test/search/*Test.cc))
TEST_OBJECTS = $(addsuffix .o, $(TEST_BINARIES)) 

## CCOPT, LINKOPT are options for compiler and linker that are used
## for both targets (release and debug).

CCOPT = -g -Wall -W -Wno-sign-compare -Wno-deprecated -ansi -pedantic -Werror -std=c++0x #-Wconversion
LINKOPT = -g
## LIBOPT: libraries
LIBOPT = -lbdd -lstdc++fs

OPT =

LBITS := $(shell getconf LONG_BIT)
ifeq ($(LBITS),64)
	OPT += -m64
else
	OPT += -m32
endif

## Additional specialized options for the various targets follow.

CCOPT_RELEASE  = -O3 -fomit-frame-pointer -DNDEBUG
CCOPT_DEBUG    = -fprofile-arcs -ftest-coverage

LINKOPT_RELEASE  = -O3 #-static -static-libgcc
LINKOPT_DEBUG    = 

CCOPT_GTEST += -I../test -DGTEST_HAS_PTHREAD=0


## Define the default target up here so that the LP stuff below
## doesn't define a default target.

default: release

test: covclear test_build

all: covclear release debug test_build test

## Build rules for the release target follow.

release: $(TARGET_RELEASE)

$(TARGET_RELEASE): $(OBJECTS_RELEASE)
	$(CC) $(LINKOPT) $(OPT) $(LINKOPT_RELEASE) $(OBJECTS_RELEASE) -o $(TARGET_RELEASE) $(LIBOPT)

$(OBJECTS_RELEASE): .obj/%$(OBJECT_SUFFIX_RELEASE).o: %.cc
	@mkdir -p $$(dirname $@)
	$(CC) $(CCOPT) $(OPT) $(CCOPT_RELEASE) -c $< -o $@

## Build rules for the debug target follow.

debug: $(TARGET_DEBUG) covclear

$(TARGET_DEBUG): $(OBJECTS_DEBUG)
	$(CC) $(LINKOPT) $(OPT) $(LINKOPT_DEBUG) $(OBJECTS_DEBUG) -o $(TARGET_DEBUG) -lgcov $(LIBOPT)

$(OBJECTS_DEBUG): .obj/%$(OBJECT_SUFFIX_DEBUG).o: %.cc
	@mkdir -p $$(dirname $@)
	$(CC) $(CCOPT) $(OPT) $(CCOPT_DEBUG) -c $< -o $@

## Build rules for test target follow

test_build:  $(TEST_MAIN).o $(GTEST_DIR)/gtest-all.o $(filter-out .obj/main.debug.o, $(OBJECTS_DEBUG)) $(TEST_OBJECTS)
				$(CC) $(CCOPT) $(OPT) -o runTests  $^ -lgcov $(LIBOPT)

$(TEST_MAIN).o: $(TEST_MAIN).cc 
				$(CC) $(filter-out -Werror, $(CCOPT)) $(OPT) -c  $< -o $@

$(GTEST_DIR)/gtest_main.o : $(GTEST_DIR)/gtest.h $(GTEST_DIR)/gtest_main.cc
	$(CC) $(CCOPT_GTEST) -g -c $(GTEST_DIR)/gtest_main.cc -o $(GTEST_DIR)/gtest_main.o

$(GTEST_DIR)/gtest-all.o : $(GTEST_DIR)/gtest.h $(GTEST_DIR)/gtest-all.cc
	$(CC) $(CCOPT_GTEST) -g -c $(GTEST_DIR)/gtest-all.cc -o $(GTEST_DIR)/gtest-all.o 

%Test.o: %Test.cc 
				$(CC) $(filter-out -Werror, $(CCOPT)) $(OPT) -c $< -o $@

## Automatically runs all tests
run_tests: 
	./runTests

# Creates code coverage html files.
coverage:
	@rm -rf ../../coverage/search
	@mkdir -p ../../coverage
	@mkdir ../../coverage/search
	@../../scripts/gcovr -r . -b -o ../../coverage/search/branch_coverage
	@../../scripts/gcovr -r . --html --html-details -o ../../coverage/search/coverage.html
	@echo "Coverage files created. Open <PROST_DIRECTORY>/coverage/search/coverage.html to view code coverage" 
	@echo "Open <PROST_DIRECTORY>/coverage/search/branch_coverage to view branch coverage"

# Deletes code coverage gcda files.
covclear:
		find .obj/ -name '*.gcda' | xargs rm -f

clean:
	rm -rf .obj
	@rm -rf ../../coverage/search
	rm -f *~ utils/*~ logical_expressions_includes/*~
	rm -f Makefile.depend
	rm -f ../test/search/*.o
	rm -f ../test/search/*.obj
	rm -f ../test/gtest/*.o

distclean: clean
	rm -f $(TARGET_RELEASE) $(TARGET_DEBUG) runTests


## Note: If we just call gcc -MM on a source file that lives within a
## subdirectory, it will strip the directory part in the output. Hence
## the for loop with the sed call.

Makefile.depend: $(SOURCES) $(HEADERS)
	rm -f Makefile.temp
	for source in $(SOURCES) ; do \
	    $(DEPEND) $$source > Makefile.temp0; \
	    objfile=$${source%%.cc}.o; \
	    sed -i -e "s@^[^:]*:@$$objfile:@" Makefile.temp0; \
	    cat Makefile.temp0 >> Makefile.temp; \
	done
	rm -f Makefile.temp0 Makefile.depend
	sed -e "s@\(.*\)\.o:\(.*\)@.obj/\1$(OBJECT_SUFFIX_RELEASE).o:\2@" Makefile.temp >> Makefile.depend
	sed -e "s@\(.*\)\.o:\(.*\)@.obj/\1$(OBJECT_SUFFIX_DEBUG).o:\2@" Makefile.temp >> Makefile.depend
	rm -f Makefile.temp

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(MAKECMDGOALS),distclean)
-include Makefile.depend
endif
endif

.PHONY: default all release debug test_build test clean distclean
