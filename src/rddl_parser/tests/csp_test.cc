#include "../../doctest/doctest.h"

#include "../csp.h"
#include "../evaluatables.h"
#include "../logical_expressions.h"
#include "../rddl.h"

using namespace std;

namespace prost {
namespace parser {
namespace tests {
TEST_CASE("RDDL Task CSP") {
    RDDLTask* task = new RDDLTask();
    auto a0 = new ActionFluent("a0", task->getType("bool"), 0);
    auto a1 = new ActionFluent("a1", task->getType("bool"), 1);
    auto a2 = new ActionFluent("a2", task->getType("bool"), 2);

    vector<Parameter*> params;
    auto pVar = new ParametrizedVariable("pv", params,
                                         ParametrizedVariable::STATE_FLUENT,
                                         task->getType("bool"), 0.0);
    task->addVariableSchematic(pVar);
    auto s0 = new StateFluent(*pVar, params, 0.0, 0);
    auto s1 = new StateFluent(*pVar, params, 0.0, 0);

    auto c0 = new ConditionalProbabilityFunction(s0, nullptr);
    auto c1 = new ConditionalProbabilityFunction(s1, nullptr);

    vector<LogicalExpression*> f0 = {new Negation(a0), new Negation(a1)};
    auto p0 = new ActionPrecondition(new Disjunction(f0));
    vector<LogicalExpression*> f1 = {new Negation(a0), new Negation(a2)};
    auto p1 = new ActionPrecondition(new Disjunction(f1));
    vector<LogicalExpression*> f2 = {new Negation(a1), new Negation(a2)};
    auto p2 = new ActionPrecondition(new Disjunction(f2));

    SUBCASE("Generation of a RDDL task CSP without preconditions") {
        // If there are no preconditions, all actions are applicable
        task->actionFluents = {a0, a1, a2};
        task->stateFluents = {s0, s1};
        task->CPFs = {c0, c1};

        RDDLTaskCSP csp(task);
        Z3Expressions const& actionVars = csp.getActionVarSet();
        Z3Expressions const& stateVars = csp.getStateVarSet();

        CHECK(actionVars.size() == 3);
        CHECK(stateVars.size() == 2);

        csp.assignActionVarSet({1, 1, 0});
        CHECK(csp.hasSolution());
    }

    SUBCASE("Generation of a RDDL task CSP with preconditions") {
        // All action variables are mutex due to the preconditions, and we test
        // an illegal action
        task->actionFluents = {a0, a1, a2};
        task->stateFluents = {s0, s1};
        task->CPFs = {c0, c1};
        task->preconds = {p0, p1, p2};

        RDDLTaskCSP csp(task);
        csp.addPreconditions();

        csp.assignActionVarSet({1, 1, 0});
        CHECK(!csp.hasSolution());
    }

    SUBCASE("Push and pop") {
        // All action variables are mutex due to the preconditions, and we first
        // test an inapplicable action, and then use pop() to reset the CSP and
        // test an applicable action
        task->actionFluents = {a0, a1, a2};
        task->stateFluents = {s0, s1};
        task->CPFs = {c0, c1};
        task->preconds = {p0, p1, p2};

        RDDLTaskCSP csp(task);
        csp.addPreconditions();

        csp.push();
        csp.assignActionVarSet({1, 1, 0});
        CHECK(!csp.hasSolution());
        csp.pop();
        csp.assignActionVarSet({1, 0, 0});
        CHECK(csp.hasSolution());
    }

    SUBCASE("Invalidate action model") {
        // We first test an applicable action, then invalidate the action model
        // and test the same action again, which is no longer applicable
        task->actionFluents = {a0, a1, a2};
        task->stateFluents = {s0, s1};
        task->CPFs = {c0, c1};
        task->preconds = {p0, p1, p2};

        RDDLTaskCSP csp(task);
        csp.addPreconditions();

        csp.assignActionVarSet({1, 0, 0});
        CHECK(csp.hasSolution());
        csp.invalidateActionModel();
        CHECK(!csp.hasSolution());
    }
}
} // namespace tests
} // namespace parser
} // namespace prost
