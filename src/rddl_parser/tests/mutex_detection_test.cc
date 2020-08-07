#include "../../doctest/doctest.h"

#include "../evaluatables.h"
#include "../logical_expressions.h"
#include "../mutex_detection.h"
#include "../rddl.h"

#include <string>

using namespace std;

namespace prost {
namespace parser {
namespace tests {
TEST_CASE("Mutex detection with binary variables") {
    auto task = new RDDLTask();
    task->numberOfConcurrentActions = 1;
    auto a0 = new ActionFluent("a0", task->getType("bool"), 0);
    auto a1 = new ActionFluent("a1", task->getType("bool"), 1);
    auto a2 = new ActionFluent("a2", task->getType("bool"), 2);

    SUBCASE("Mutex detection with a single action variable") {
        // If there is only one action variable, that action variable is not
        // mutex with another action variable
        task->actionFluents = {a0};
        TaskMutexInfo mutexInfo = computeActionVarMutexes(task);
        CHECK(mutexInfo.size() == 1);
        CHECK(!mutexInfo.hasMutexVarPair());
        CHECK(!mutexInfo[a0].isMutexWithSomeVar());
    }

    SUBCASE("Variables are not mutex with themselves") {
        // If there is only one action variable, that action variable is not
        // mutex with another action variable
        task->actionFluents = {a0, a1, a2};
        TaskMutexInfo mutexInfo = computeActionVarMutexes(task);
        CHECK(!mutexInfo[a0].isMutexWith(a0));
        CHECK(!mutexInfo[a1].isMutexWith(a1));
        CHECK(!mutexInfo[a2].isMutexWith(a2));
    }

    SUBCASE("Mutex detection without concurrency and preconditions") {
        // If there is no concurrency, all pairs of action variables are mutex
        task->actionFluents = {a0, a1, a2};
        TaskMutexInfo mutexInfo = computeActionVarMutexes(task);

        CHECK(mutexInfo.size() == 3);
        CHECK(mutexInfo.hasMutexVarPair());
        CHECK(mutexInfo.allVarsArePairwiseMutex());
        CHECK(mutexInfo[a0].isMutexWithAllVars());
        CHECK(mutexInfo[a1].isMutexWithAllVars());
        CHECK(mutexInfo[a2].isMutexWithAllVars());
    }

    SUBCASE("Mutex detection with concurrency and without preconditions") {
        // If there is concurrency but no preconditions, no pair of action
        // variables can be mutex
        task->numberOfConcurrentActions = 2;
        task->actionFluents = {a0, a1, a2};
        TaskMutexInfo mutexInfo = computeActionVarMutexes(task);

        CHECK(mutexInfo.size() == 3);
        CHECK(!mutexInfo.hasMutexVarPair());
        CHECK(!mutexInfo.allVarsArePairwiseMutex());
        CHECK(!mutexInfo[a0].isMutexWithSomeVar());
        CHECK(!mutexInfo[a1].isMutexWithSomeVar());
        CHECK(!mutexInfo[a2].isMutexWithSomeVar());
    }

    SUBCASE("Mutex detection with concurrency and preconditions") {
        // There are three action variables and two preconditions such that a0
        // and a1 as well as a0 and a2 are mutex whereas a1 and a2 are not
        task->numberOfConcurrentActions = 2;
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
        CHECK(mutexInfo.hasMutexVarPair());
        CHECK(!mutexInfo.allVarsArePairwiseMutex());
        CHECK(mutexInfo[a0].isMutexWithSomeVar());
        CHECK(mutexInfo[a0].isMutexWithAllVars());
        CHECK(mutexInfo[a1].isMutexWithSomeVar());
        CHECK(!mutexInfo[a1].isMutexWithAllVars());
        CHECK(mutexInfo[a1].isMutexWith(a0));
        CHECK(!mutexInfo[a1].isMutexWith(a2));
        CHECK(mutexInfo[a2].isMutexWithSomeVar());
        CHECK(!mutexInfo[a2].isMutexWithAllVars());
        CHECK(mutexInfo[a2].isMutexWith(a0));
    }
}

TEST_CASE("Mutex detection with FDR variables") {
    // FDR variables are ignored by mutex detection
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

    TaskMutexInfo mutexInfo = computeActionVarMutexes(task);

    CHECK(mutexInfo.size() == 2);
    CHECK(!mutexInfo.hasMutexVarPair());
    CHECK(!mutexInfo[a0].isMutexWithSomeVar());
    CHECK(!mutexInfo[a1].isMutexWithSomeVar());
}
} // namespace tests
} // namespace parser
} // namespace prost
