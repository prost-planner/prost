# Checking if BuDDy is installed with libbdd-dev 
# Once done, this will define
#
#  BDD_FOUND - system has bdd installed

# TODO: Solve this with modules - that would enable user to install BuDDy where ever
# TODO: Check if the OS is Windows - in that case, do not include BuDDy
#if (NOT EXISTS "/usr/include/bdd.h")
#  set(BDD_FOUND false)
#else(EXISTS "/usr/include/bdd.h")
#  set(BDD_FOUND true)
#endif()

# Include mModule, where the macro is declared.
include(CheckIncludeFile)
# Check that header file is accessible.
CHECK_INCLUDE_FILE("bdd.h" # "Signature" header for the library libbdd.
    BDD_FOUND # Variable to store result
)
if(NOT BDD_FOUND)
    # Remove FALSE value from the cache, so next run
    # the include file will be rechecked.
    #
    # Without removing, CHECK_INCLUDE_FILE will check nothing at next run.
    unset(BDD_FOUND CACHE)

    message(SEND_ERROR "Unable to detect 'bdd' library. Install libbdd-dev and rerun cmake.")
endif(NOT BDD_FOUND)

# Assume that libbdd is installed.
