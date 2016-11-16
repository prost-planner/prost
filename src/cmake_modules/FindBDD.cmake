# Checking if BuDDy is instaled with libbdd-dev 
# Once done, this will define
#
#  BDD_FOUND - system has bdd installed

# TODO: Solve this with modules - that would enable user to install BuDDy where ever
# TODO: Check if the OS is Windows - in that case, do not include BuDDy
if (NOT EXISTS "/usr/include/bdd.h")
  set(BDD_FOUND false)
else(EXISTS "/usr/include/bdd.h")
  set(BDD_FOUND true)
endif()