HEADERS = parser.h \
	  prost_planner.h \
	  search_engine.h \
	  thts.h \
	  uct_base.h \
	  mc_uct_search.h \
	  max_mc_uct_search.h \
	  dp_uct_search.h \
	  depth_first_search.h \
	  iterative_deepening_search.h \
	  uniform_evaluation_search.h \
	  ippc_client.h \
	  states.h \
	  evaluatables.h \
	  logical_expressions.h \
	  probability_distribution.h \
	  utils/strxml.h \
	  utils/timer.h \
	  utils/string_utils.h \
	  utils/system_utils.h \
	  utils/math_utils.h \

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
DEPEND = g++ -MM

## Testfile binaries
TEST_BINARIES = $(basename $(wildcard test/*Test.cc))

## CCOPT, LINKOPT are options for compiler and linker that are used
## for both targets (release and debug).

CCOPT = -g -Wall -W -Wno-sign-compare -Wno-deprecated -ansi -pedantic -Werror #-Wconversion
LINKOPT = -g

OPT =

LBITS := $(shell getconf LONG_BIT)
ifeq ($(LBITS),64)
	OPT += -m64
else
	OPT += -m32
endif

## Additional specialized options for the various targets follow.

CCOPT_RELEASE  = -O3 -fomit-frame-pointer -DNDEBUG
CCOPT_DEBUG    = 

LINKOPT_RELEASE  = -O3 #-static -static-libgcc
LINKOPT_DEBUG    =

## Define the default target up here so that the LP stuff below
## doesn't define a default target.

default: debug

all: release debug

flo: debug test_build test

## Build rules for the release target follow.

release: $(TARGET_RELEASE)

$(TARGET_RELEASE): $(OBJECTS_RELEASE)
	$(CC) $(LINKOPT) $(OPT) $(LINKOPT_RELEASE) $(OBJECTS_RELEASE) -o $(TARGET_RELEASE) -lbdd

$(OBJECTS_RELEASE): .obj/%$(OBJECT_SUFFIX_RELEASE).o: %.cc
	@mkdir -p $$(dirname $@)
	$(CC) $(CCOPT) $(OPT) $(CCOPT_RELEASE) -c $< -o $@

## Build rules for the debug target follow.

debug: $(TARGET_DEBUG)

$(TARGET_DEBUG): $(OBJECTS_DEBUG)
	$(CC) $(LINKOPT) $(OPT) $(LINKOPT_DEBUG) $(OBJECTS_DEBUG) -o $(TARGET_DEBUG) -lbdd

$(OBJECTS_DEBUG): .obj/%$(OBJECT_SUFFIX_DEBUG).o: %.cc
	@mkdir -p $$(dirname $@)
	$(CC) $(CCOPT) $(OPT) $(CCOPT_DEBUG) -c $< -o $@

## Build rules for test target follow

test_build: $(TEST_BINARIES)

%Test: %Test.o $(filter-out .obj/main.debug.o, $(OBJECTS_DEBUG))
				$(CC) $(CCOPT) $(OPT) $(CCOPT_DEBUG) -o $@ $^ -lgtest -lgtest_main -lpthread -lbdd

%Test.o: %Test.cc 
				$(CC) $(filter-out -Werror, $(CCOPT)) $(OPT) $(CCOPT_DEBUG) -c $< -o $@

## Automatically runs all tests from TEST_BINARIES.
test: $(TEST_BINARIES)
				for T in $(TEST_BINARIES); do ./$$T; done


clean:
	rm -rf .obj
	rm -f *~ utils/*~ logical_expressions_includes/*~
	rm -f Makefile.depend
	rm -f test/*.o
	rm -f test/*.obj
	rm -f test/$(basename $(notdir $(TEST_BINARIES)))


distclean: clean
	rm -f $(TARGET_RELEASE) $(TARGET_DEBUG)

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

.PHONY: default all release debug flo test_build test clean distclean
