#include "../../doctest/doctest.h"

#include "../prost_planner.h"

// This is the main test fixture class for all unit tests. It automaticall
// resets all static members between each test, so that the user does not have
// to do this manually.
class ProstUnitTest {
public:
    ProstUnitTest() {
        ProstPlanner::resetStaticMembers();
    }
};

