#include "../../doctest/doctest.h"

#include "../fdr_generation.h"
#include "../logical_expressions.h"
#include "../mutex_detection.h"
#include "../rddl.h"

using namespace std;

TEST_CASE("Greedy FDR generation") {
    auto task = new RDDLTask();
    auto a0 = new ActionFluent("a0", task->getType("bool"), 0);
    auto a1 = new ActionFluent("a1", task->getType("bool"), 1);
    auto a2 = new ActionFluent("a2", task->getType("bool"), 2);
    auto a3 = new ActionFluent("a3", task->getType("bool"), 3);
    auto a4 = new ActionFluent("a4", task->getType("bool"), 4);
    Simplifications dummy;
    GreedyFDRGenerator fdrGen(task);

    SUBCASE("Greedy FDR generation when no pair of variables is mutex") {
        task->actionFluents = {a0, a1, a2};
        TaskMutexInfo mutexInfo(task);

        vector<ActionFluent*> fdrVars = fdrGen.generateFDRVars(mutexInfo, dummy);
        CHECK(fdrVars.size() == 3);
        CHECK(fdrVars[0] == a0);
        CHECK(fdrVars[1] == a1);
        CHECK(fdrVars[2] == a2);
    }

    SUBCASE("FDR generation when all variables are pairwise mutex") {
        task->actionFluents = {a0, a1, a2};
        TaskMutexInfo mutexInfo(task);
        mutexInfo.addMutexInfo(a0, a1);
        mutexInfo.addMutexInfo(a0, a2);
        mutexInfo.addMutexInfo(a1, a2);

        vector<ActionFluent*> fdrVars = fdrGen.generateFDRVars(mutexInfo, dummy);
        CHECK(fdrVars.size() == 1);
        CHECK(fdrVars[0]->isFDR);
        CHECK(fdrVars[0]->domainSize() == 4);
    }

    SUBCASE("FDR generation when some variable pairs are mutex") {
        // Of the five action variables, the first 3 are pairwise mutex and the
        // last two are. There are other mutex pairs, but not in a way that
        // these sets of pairwise mutex variables can be extended, so the result
        // are two FDR variables.
        task->actionFluents = {a0, a1, a2, a3, a4};
        TaskMutexInfo mutexInfo(task);
        mutexInfo.addMutexInfo(a0, a1);
        mutexInfo.addMutexInfo(a0, a2);
        mutexInfo.addMutexInfo(a1, a2);
        mutexInfo.addMutexInfo(a1, a3);
        mutexInfo.addMutexInfo(a1, a4);
        mutexInfo.addMutexInfo(a3, a4);

        vector<ActionFluent*> fdrVars = fdrGen.generateFDRVars(mutexInfo, dummy);
        CHECK(fdrVars.size() == 2);
        CHECK(fdrVars[0]->isFDR);
        CHECK(fdrVars[0]->domainSize() == 4);
        CHECK(fdrVars[1]->isFDR);
        CHECK(fdrVars[1]->domainSize() == 3);
    }
}
