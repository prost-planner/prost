#include "../../doctest/doctest.h"

#include "../evaluatables.h"
#include "../logical_expressions.h"
#include "../mutex_detection.h"
#include "../rddl.h"

#include <string>

using namespace std;

TEST_CASE("Mutex detection without concurrency") {
    // If there is no concurrency, all pairs of action fluents are mutex
    auto task = new RDDLTask();
    task->numberOfConcurrentActions = 1;
    auto a0 = new ActionFluent("a0", task->getType("bool"), 0);
    auto a1 = new ActionFluent("a1", task->getType("bool"), 1);
    auto a2 = new ActionFluent("a2", task->getType("bool"), 2);
    task->actionFluents = {a0, a1, a2};

    TaskMutexInfo mutexInfo = computeActionVarMutexes(task);

    CHECK(mutexInfo.size() == 3);
    CHECK(mutexInfo.hasMutex());
    CHECK(mutexInfo.allVarsArePairwiseMutex());
    CHECK(mutexInfo[0].isMutexWithAllVars());
    CHECK(mutexInfo[1].isMutexWithAllVars());
    CHECK(mutexInfo[2].isMutexWithAllVars());
}

TEST_CASE("Mutex detection without preconditions") {
    // If concurrency is allowed (any number x larger than 1) and there are no
    // preconditions, any x-tuple of fluents is applicable concurrently
    auto task = new RDDLTask();
    task->numberOfConcurrentActions = 2;
    auto a0 = new ActionFluent("a0", task->getType("bool"), 0);
    auto a1 = new ActionFluent("a1", task->getType("bool"), 1);
    auto a2 = new ActionFluent("a2", task->getType("bool"), 2);
    task->actionFluents = {a0, a1, a2};

    TaskMutexInfo mutexInfo = computeActionVarMutexes(task);

    CHECK(mutexInfo.size() == 3);
    CHECK(!mutexInfo.hasMutex());
    CHECK(!mutexInfo.allVarsArePairwiseMutex());
    CHECK(!mutexInfo[0].hasMutex());
    CHECK(!mutexInfo[1].hasMutex());
    CHECK(!mutexInfo[2].hasMutex());
}

TEST_CASE("Mutex detection with a single action fluent") {
    // If there is only one action fluent, that action fluent is not mutex with
    // another action fluent
    auto task = new RDDLTask();
    task->numberOfConcurrentActions = 2;
    auto a0 = new ActionFluent("a0", task->getType("bool"), 0);
    task->actionFluents = {a0};

    TaskMutexInfo mutexInfo = computeActionVarMutexes(task);

    CHECK(mutexInfo.size() == 1);
    CHECK(!mutexInfo.hasMutex());
    CHECK(!mutexInfo[0].hasMutex());
}

TEST_CASE("Mutex detection with FDR variables") {
    // If there is only one action fluent, that action fluent is not mutex with
    // another action fluent
    auto task = new RDDLTask();
    task->numberOfConcurrentActions = 2;
    Type* varType = task->addType("dummy");
    task->addObject("dummy", "0");
    task->addObject("dummy", "1");
    task->addObject("dummy", "2");
    task->addObject("dummy", "3");
    auto a0 = new ActionFluent("a0", varType, 0);
    auto a1 = new ActionFluent("a1", varType, 1);
    task->actionFluents = {a0, a1};
    CHECK(a0->isFDR);
    CHECK(a1->isFDR);
    CHECK(a0->valueType->objects.size() == 4);

    TaskMutexInfo mutexInfo = computeActionVarMutexes(task);

    CHECK(mutexInfo.size() == 2);
    CHECK(!mutexInfo.hasMutex());
    CHECK(!mutexInfo[0].hasMutex());
    CHECK(!mutexInfo[1].hasMutex());
}

TEST_CASE("Mutex detection with relevant preconditions") {
    // There are three fluents and two preconditions such that a0 and a1 as
    // well as a0 and a2 are mutex
    auto task = new RDDLTask();
    task->numberOfConcurrentActions = 2;
    auto a0 = new ActionFluent("a0", task->getType("bool"), 0);
    auto a1 = new ActionFluent("a1", task->getType("bool"), 1);
    auto a2 = new ActionFluent("a2", task->getType("bool"), 2);
    task->actionFluents = {a0, a1, a2};

    // Build precondition (a0 + a1) <= 1
    vector<LogicalExpression*> a0a1 = {a0, a1};
    vector<LogicalExpression*> leqa0a1 =
        {new Addition(a0a1), new NumericConstant(1)};
    auto p1 = new ActionPrecondition(new LowerEqualsExpression(leqa0a1));

    // Build precondition \neg(a0 \land a2)
    vector<LogicalExpression*> a0a2 = {a0, a2};
    auto p2 = new ActionPrecondition(new Negation(new Conjunction(a0a2)));
    task->preconds = {p1, p2};

    TaskMutexInfo mutexInfo = computeActionVarMutexes(task);

    CHECK(mutexInfo.size() == 3);
    CHECK(mutexInfo.hasMutex());
    CHECK(!mutexInfo.allVarsArePairwiseMutex());
    CHECK(mutexInfo[0].hasMutex());
    CHECK(mutexInfo[0].isMutexWithAllVars());
    CHECK(mutexInfo[1].hasMutex());
    CHECK(!mutexInfo[1].isMutexWithAllVars());
    CHECK(mutexInfo[1].isMutexWith(0));
    CHECK(mutexInfo[2].hasMutex());
    CHECK(!mutexInfo[2].isMutexWithAllVars());
    CHECK(mutexInfo[2].isMutexWith(0));
}

