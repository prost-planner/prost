HEADERS = prost_planner.h \
	  search_engine.h \
	  uct_search.h \
	  depth_first_search.h \
	  iterative_deepening_search.h \
	  random_search.h \
	  state_set_generator.h \
          ippc_client.h \
	  rddl_parser.h \
	  instantiator.h \
	  preprocessor.h \
	  state.h \
	  planning_task.h \
	  unprocessed_planning_task.h \
	  typed_objects.h \
	  logical_expressions.h \
	  actions.h \
	  conditional_probability_functions.h \
	  caching_component.h \
	  learning_component.h \
	  utils/strxml.h \
          utils/timer.h \
          utils/string_utils.h \
          utils/system_utils.h \
	  utils/math_utils.h \

SOURCES = main.cc \
	  prost_planner.cc \
	  search_engine.cc \
	  uct_search.cc \
	  depth_first_search.cc \
	  iterative_deepening_search.cc \
	  random_search.cc \
	  state_set_generator.cc \
	  rddl_parser.cc \
	  instantiator.cc \
	  preprocessor.cc \
	  planning_task.cc \
	  unprocessed_planning_task.cc \
          ippc_client.cc \
	  typed_objects.cc \
	  logical_expressions.cc \
	  actions.cc \
	  conditional_probability_functions.cc \
	  caching_component.cc \
	  learning_component.cc \
	  utils/strxml.cc \
          utils/timer.cc \
          utils/string_utils.cc \
          utils/system_utils.cc \
	  utils/math_utils.cc \

TARGET  = prost

OBJECT_SUFFIX_RELEASE =
TARGET_SUFFIX_RELEASE =
OBJECT_SUFFIX_DEBUG   = .debug
TARGET_SUFFIX_DEBUG   = -debug

OBJECT_SUFFIX_RELEASE64 = .release64
TARGET_SUFFIX_RELEASE64 = 64
OBJECT_SUFFIX_DEBUG64   = .debug64
TARGET_SUFFIX_DEBUG64   = 64-debug

OBJECTS_RELEASE = $(SOURCES:%.cc=.obj/%$(OBJECT_SUFFIX_RELEASE).o)
TARGET_RELEASE  = $(TARGET)$(TARGET_SUFFIX_RELEASE)

OBJECTS_RELEASE64 = $(SOURCES:%.cc=.obj/%$(OBJECT_SUFFIX_RELEASE64).o)
TARGET_RELEASE64  = $(TARGET)$(TARGET_SUFFIX_RELEASE64)

OBJECTS_DEBUG   = $(SOURCES:%.cc=.obj/%$(OBJECT_SUFFIX_DEBUG).o)
TARGET_DEBUG    = $(TARGET)$(TARGET_SUFFIX_DEBUG)

OBJECTS_DEBUG64   = $(SOURCES:%.cc=.obj/%$(OBJECT_SUFFIX_DEBUG64).o)
TARGET_DEBUG64    = $(TARGET)$(TARGET_SUFFIX_DEBUG64)

CC     = g++
DEPEND = g++ -MM

## CCOPT, LINKOPT are options for compiler and linker that are used
## for all targets (release, debug, relaease64 and debug64).

CCOPT = -g -Wall -W -Wno-sign-compare -Wno-deprecated -ansi -pedantic -Werror #-Wconversion
LINKOPT = -g -lbdd

OPT32 = -m32
OPT64 = -m64

## Additional specialized options for the various targets follow.
## In release mode, we link statically since this makes it more likely
## that local compiles will work on the grid
##
## NOTE: This precludes some uses of exceptions.
## For details, see man gcc on -static-libgcc.

CCOPT_RELEASE  = -O3 -fomit-frame-pointer
CCOPT_DEBUG    = 

LINKOPT_RELEASE  = -O3 -static -static-libgcc
LINKOPT_DEBUG    =

## Define the default target up here so that the LP stuff below
## doesn't define a default target.

default: debug

all: release debug

all64: release64 debug64

## Build rules for the release target follow.

release: $(TARGET_RELEASE)

$(TARGET_RELEASE): $(OBJECTS_RELEASE)
	$(CC) $(LINKOPT) $(OPT32) $(LINKOPT_RELEASE) $(OBJECTS_RELEASE) -o $(TARGET_RELEASE) -lbdd

$(OBJECTS_RELEASE): .obj/%$(OBJECT_SUFFIX_RELEASE).o: %.cc
	@mkdir -p $$(dirname $@)
	$(CC) $(CCOPT) $(OPT32) $(CCOPT_RELEASE) -c $< -o $@


release64: $(TARGET_RELEASE64)

$(TARGET_RELEASE64): $(OBJECTS_RELEASE64)
	$(CC) $(LINKOPT) $(OPT64) $(LINKOPT_RELEASE) $(OBJECTS_RELEASE64) -o $(TARGET_RELEASE64) -lbdd

$(OBJECTS_RELEASE64): .obj/%$(OBJECT_SUFFIX_RELEASE64).o: %.cc
	@mkdir -p $$(dirname $@)
	$(CC) $(CCOPT) $(OPT64) $(CCOPT_RELEASE) -c $< -o $@

## Build rules for the debug target follow.

debug: $(TARGET_DEBUG)

$(TARGET_DEBUG): $(OBJECTS_DEBUG)
	$(CC) $(LINKOPT) $(OPT32) $(LINKOPT_DEBUG) $(OBJECTS_DEBUG) -o $(TARGET_DEBUG) -lbdd

$(OBJECTS_DEBUG): .obj/%$(OBJECT_SUFFIX_DEBUG).o: %.cc
	@mkdir -p $$(dirname $@)
	$(CC) $(CCOPT) $(OPT32) $(CCOPT_DEBUG) -c $< -o $@

debug64: $(TARGET_DEBUG64)

$(TARGET_DEBUG64): $(OBJECTS_DEBUG64)
	$(CC) $(LINKOPT) $(OPT64) $(LINKOPT_DEBUG) $(OBJECTS_DEBUG64) -o $(TARGET_DEBUG64) -lbdd

$(OBJECTS_DEBUG64): .obj/%$(OBJECT_SUFFIX_DEBUG64).o: %.cc
	@mkdir -p $$(dirname $@)
	$(CC) $(CCOPT) $(OPT64) $(CCOPT_DEBUG) -c $< -o $@

clean:
	rm -rf .obj
	rm -f *~ utils/*~ logical_expressions_includes/*~
	rm -f Makefile.depend

distclean: clean
	rm -f $(TARGET_RELEASE) $(TARGET_DEBUG) $(TARGET_RELEASE64) $(TARGET_DEBUG64)

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
	sed -e "s@\(.*\)\.o:\(.*\)@.obj/\1$(OBJECT_SUFFIX_RELEASE64).o:\2@" Makefile.temp >> Makefile.depend
	sed -e "s@\(.*\)\.o:\(.*\)@.obj/\1$(OBJECT_SUFFIX_DEBUG64).o:\2@" Makefile.temp >> Makefile.depend
	rm -f Makefile.temp

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(MAKECMDGOALS),distclean)
-include Makefile.depend
endif
endif

.PHONY: default all release debug clean distclean
