#include "../../doctest/doctest.h"

#include "../fdr_generation.h"
#include "../logical_expressions.h"
#include "../mutex_detection.h"
#include "../rddl.h"

#include <memory>

using namespace std;

TEST_CASE("Greedy FDR generation when no pair of variables is mutex") {
    RDDLTask* task = new RDDLTask();
    ActionFluent* a0 = new ActionFluent("a0", task->getType("bool"), 0);
    ActionFluent* a1 = new ActionFluent("a1", task->getType("bool"), 1);
    ActionFluent* a2 = new ActionFluent("a2", task->getType("bool"), 2);
    task->actionFluents = {a0, a1, a2};
    TaskMutexInfo mutexInfo(task);
    CHECK(!mutexInfo.hasMutex());

    Simplifications dummy;
    GreedyFDRGenerator fdrGen(task);
    vector<ActionFluent*> fdrVars = fdrGen.generateFDRVars(mutexInfo, dummy);
    CHECK(fdrVars.size() == 3);
    CHECK(fdrVars[0] == a0);
    CHECK(fdrVars[1] == a1);
    CHECK(fdrVars[2] == a2);
}

TEST_CASE("FDR generation when all variables are pairwise mutex") {
    RDDLTask* task = new RDDLTask();
    ActionFluent* a0 = new ActionFluent("a0", task->getType("bool"), 0);
    ActionFluent* a1 = new ActionFluent("a1", task->getType("bool"), 1);
    ActionFluent* a2 = new ActionFluent("a2", task->getType("bool"), 2);
    task->actionFluents = {a0, a1, a2};
    TaskMutexInfo mutexInfo(task);
    mutexInfo.varsAreMutex(0,1);
    mutexInfo.varsAreMutex(0,2);
    mutexInfo.varsAreMutex(1,2);
    CHECK(mutexInfo.allVarsArePairwiseMutex());

    Simplifications dummy;
    GreedyFDRGenerator fdrGen(task);
    vector<ActionFluent*> fdrVars = fdrGen.generateFDRVars(mutexInfo, dummy);
    CHECK(fdrVars.size() == 1);
    CHECK(fdrVars[0]->isFDR);
    CHECK(fdrVars[0]->domainSize() == 4);
}

TEST_CASE("FDR generation when some variable pairs are mutex") {
    // If there is no concurrency, all pairs of action fluents are mutex
    RDDLTask* task = new RDDLTask();
    ActionFluent* a0 = new ActionFluent("a0", task->getType("bool"), 0);
    ActionFluent* a1 = new ActionFluent("a1", task->getType("bool"), 1);
    ActionFluent* a2 = new ActionFluent("a2", task->getType("bool"), 2);
    ActionFluent* a3 = new ActionFluent("a3", task->getType("bool"), 3);
    ActionFluent* a4 = new ActionFluent("a4", task->getType("bool"), 4);
    task->actionFluents = {a0, a1, a2, a3, a4};
    TaskMutexInfo mutexInfo(task);
    mutexInfo.varsAreMutex(0,1);
    mutexInfo.varsAreMutex(0,2);
    mutexInfo.varsAreMutex(1,2);
    mutexInfo.varsAreMutex(1,3);
    mutexInfo.varsAreMutex(1,4);
    mutexInfo.varsAreMutex(3,4);

    shared_ptr<FDRGenerator> test = make_shared<GreedyFDRGenerator>(task);

    Simplifications dummy;
    GreedyFDRGenerator fdrGen(task);
    vector<ActionFluent*> fdrVars = fdrGen.generateFDRVars(mutexInfo, dummy);
    CHECK(fdrVars.size() == 2);
    CHECK(fdrVars[0]->isFDR);
    CHECK(fdrVars[0]->domainSize() == 4);
    CHECK(fdrVars[1]->isFDR);
    CHECK(fdrVars[1]->domainSize() == 3);
}
