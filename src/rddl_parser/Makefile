HEADERS = rddl.h \
	  instantiator.h \
	  preprocessor.h \
	  task_analyzer.h \
	  evaluatables.h \
	  logical_expressions.h \
	  states.h \
	  probability_distribution.h \
	  utils/timer.h \
	  utils/string_utils.h \
	  utils/system_utils.h \
	  utils/math_utils.h \

SOURCES = $(HEADERS:%.h=%.cc)

SHELL = /bin/bash

TARGET  = rddl-parser

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

LEX = flex
YACC = bison
YFLAGS = -d

## Test related
GTEST_DIR = ../test/gtest
TEST_MAIN = ../test/testMain
TEST_BINARIES = $(basename $(wildcard ../test/rddl_parser/*Test.cc))
TEST_OBJECTS = $(addsuffix .o, $(TEST_BINARIES))

## CCOPT, LINKOPT are options for compiler and linker that are used
## for both targets (release and debug).

CCOPT = -g -Wall -W -Wno-sign-compare -Wno-deprecated -ansi -pedantic -Werror -std=c++0x #-Wconversion
LINKOPT = -g

OPT = -std=c++0x

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

test: covclear test_build run_tests

all: covclear release debug test_build test

## Build rules for the release target follow.

release: $(TARGET_RELEASE)

$(TARGET_RELEASE): $(OBJECTS_RELEASE) parser.ypp lexer.l
	## Bison and flex generation rules
	bison -d parser.ypp --output-file parser.tab.cc
	flex lexer.l parser.tab.hh
	g++ -std=c++11 -c -o .obj/lex.yy.o lex.yy.c
	g++ -std=c++11 -c -o parser.tab.o parser.tab.cc
	##
	$(CC) $(LINKOPT) $(OPT) $(LINKOPT_RELEASE) $(OBJECTS_RELEASE) parser.tab.o .obj/lex.yy.o -o $(TARGET_RELEASE)

$(OBJECTS_RELEASE): .obj/%$(OBJECT_SUFFIX_RELEASE).o: %.cc
	@mkdir -p $$(dirname $@)
	$(CC) $(CCOPT) $(OPT) $(CCOPT_RELEASE) -c $< -o $@

## Build rules for the debug target follow.

debug: $(TARGET_DEBUG) covclear

$(TARGET_DEBUG): $(OBJECTS_DEBUG) parser.ypp lexer.l
	## Bison and flex generation rules
	bison -d parser.ypp --output-file parser.tab.cc
	flex lexer.l parser.tab.hh
	g++ -std=c++11 -c -o .obj/lex.yy.o lex.yy.c
	g++ -std=c++11 -c -o parser.tab.o parser.tab.cc
	##
	$(CC) $(LINKOPT) $(OPT) $(LINKOPT_DEBUG) $(OBJECTS_DEBUG) parser.tab.o .obj/lex.yy.o -o $(TARGET_DEBUG) -lgcov

$(OBJECTS_DEBUG): .obj/%$(OBJECT_SUFFIX_DEBUG).o: %.cc
	@mkdir -p $$(dirname $@)
	$(CC) $(CCOPT) $(OPT) $(CCOPT_DEBUG) -c $< -o $@

## Build rules for test target follow

test_build:  $(TEST_MAIN).o $(GTEST_DIR)/gtest-all.o $(filter-out .obj/main.debug.o, $(OBJECTS_DEBUG)) $(TEST_OBJECTS)
				$(CC) $(CCOPT) $(OPT) -o runTests  $^ -lgcov

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
	@rm -rf ../../coverage/rddl_parser
	@mkdir -p ../../coverage
	@mkdir ../../coverage/rddl_parser
	@../../scripts/gcovr -r . -b -o ../../coverage/rddl_parser/branch_coverage
	@../../scripts/gcovr -r . --html --html-details -o ../../coverage/rddl_parser/coverage.html
	@echo "Coverage files created. Open <PROST_DIRECTORY>/coverage/rddl_parser/coverage.html to view code coverage"
	@echo "Open <PROST_DIRECTORY>/coverage/rddl_parser/branch_coverage to view branch coverage"

# Deletes code coverage gcda files.
covclear:
		find .obj/ -name '*.gcda' | xargs rm -f
clean:
	rm -rf .obj
	rm -rf ../../coverage/rddl_parser
	rm -f *~ utils/*~ logical_expressions_includes/*~
	rm -f Makefile.depend
	rm -f ../test/rddl_parser/*.o
	rm -f ../test/rddl_parser/*.obj
	rm -f ../test/gtest/*.o
	rm -rf *~ $(PROGRAM) *.tab.* lex.* \.combined

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
