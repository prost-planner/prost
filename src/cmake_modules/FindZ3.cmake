# - Find the Z3 SMT solver.
# This code defines the following variables:
#
#  Z3_FOUND                 - TRUE if Z3 was found.
#  Z3_INCLUDE_DIRS          - Full paths to all include dirs.
#  Z3_LIBRARIES             - Full paths to all libraries.
#
# Usage:
#  find_package(Z3)
#
# Hints to the location of Z3 can be given with the cmake variable
# Z3_ROOT or the environment variable Z3_ROOT

set(Z3_HINT_PATHS
    ${Z3_ROOT}
    $ENV{Z3_ROOT}
)

find_path(Z3_INCLUDE_DIRS
    NAMES "z3++.h"
    HINTS ${Z3_HINT_PATHS}
    PATH_SUFFIXES include
)

find_library(Z3_LIBRARIES
    NAMES z3
    HINTS ${Z3_HINT_PATHS}
    PATH_SUFFIXES lib
)

# Check if everything was found and set Z3_FOUND.
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    Z3
    REQUIRED_VARS Z3_INCLUDE_DIRS Z3_LIBRARIES
)

mark_as_advanced(Z3_INCLUDE_DIRS, Z3_LIBRARIES)
