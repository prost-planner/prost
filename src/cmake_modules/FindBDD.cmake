# - Find the BuDDY BDD library.
# This code defines the following variables:
#
#  BDD_FOUND                - TRUE if BDD was found.
#  BDD_INCLUDE_DIRS         - Full paths to all include dirs.
#  BDD_LIBRARIES            - Full paths to all libraries.
#
# Usage:
#  find_package(BDD)
#
# Hints to the location of BuDDy can be given with the cmake variable
# BDD_ROOT or the environment variable BDD_ROOT. If BuDDy is installed
# with apt, it should be found automatically without providing a hint.

set(BDD_HINT_PATHS
    ${BDD_ROOT}
    $ENV{BDD_ROOT}
)

find_path(BDD_INCLUDE_DIRS
    NAMES "bdd.h"
    HINTS ${BDD_HINT_PATHS}
    PATH_SUFFIXES include
)

find_library(BDD_LIBRARIES
    NAMES bdd
    HINTS ${BDD_HINT_PATHS}
    PATH_SUFFIXES lib
)

# Check if everything was found and set BDD_FOUND.
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    BDD
    REQUIRED_VARS BDD_INCLUDE_DIRS BDD_LIBRARIES
)

mark_as_advanced(BDD_INCLUDE_DIRS, BDD_LIBRARIES)



