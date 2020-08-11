#include "../../doctest/doctest.h"

#include "../csp.h"
#include "../evaluatables.h"
#include "../logical_expressions.h"
#include "../rddl.h"

using namespace std;

namespace prost::parser {
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

        vector<vector<int>> actions = {{0,0,0}, {0,0,1}, {0,1,0}, {0,1,1},
                                       {1,0,0}, {1,0,1}, {1,1,0}, {1,1,1}};
        for (vector<int> const& action : actions) {
            csp.push();
            csp.assignActionVarSet(action);
            CHECK(csp.hasSolution());
            csp.pop();
        }
    }

    vector<LogicalExpression*> f0 = {new Negation(a0), new Negation(a1)};
    auto p0 = new ActionPrecondition(new Disjunction(f0));
    vector<LogicalExpression*> f1 = {new Negation(a0), new Negation(a2)};
    auto p1 = new ActionPrecondition(new Disjunction(f1));
    vector<LogicalExpression*> f2 = {new Negation(a1), new Negation(a2)};
    auto p2 = new ActionPrecondition(new Disjunction(f2));

    SUBCASE("Generation of a RDDL task CSP with preconditions") {
        // All action variables are mutex due to the preconditions, and we test
        // an illegal action
        task->actionFluents = {a0, a1, a2};
        task->stateFluents = {s0, s1};
        task->CPFs = {c0, c1};
        task->preconds = {p0, p1, p2};

        RDDLTaskCSP csp(task);
        csp.addPreconditions();

        vector<vector<int>> legalActions =
            {{0,0,0}, {0,0,1}, {0,1,0},  {1,0,0}};
        for (vector<int> const& action : legalActions) {
            csp.push();
            csp.assignActionVarSet(action);
            CHECK(csp.hasSolution());
            csp.pop();
        }

        vector<vector<int>> illegalActions =
            {{0,1,1}, {1,0,1}, {1,1,0}, {1,1,1}};
        for (vector<int> const& action : illegalActions) {
            csp.push();
            csp.assignActionVarSet(action);
            CHECK(!csp.hasSolution());
            csp.pop();
        }

        csp.assignActionVarSet({1, 1, 0});
        CHECK(!csp.hasSolution());
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

    SUBCASE("Multiple sets of action variables") {
        // We have preconditions a0 => s0 and a1 => not s0 and check that action
        // {1, 0} is applicable in some states and action {0, 1} is applicable
        // in some state, but that there is no state where both {1, 0} and
        // {0, 1} are applicable. To do so, we need a second set of action
        // variables.
        vector<LogicalExpression*> f3 = {new Negation(a0), s0};
        vector<LogicalExpression*> f4 = {new Negation(a1), new Negation(s0)};
        auto p3 = new ActionPrecondition(new Disjunction(f3));
        auto p4 = new ActionPrecondition(new Disjunction(f4));

        task->actionFluents = {a0, a1};
        task->stateFluents = {s0};
        task->CPFs = {c0};
        task->preconds = {p3, p4};

        RDDLTaskCSP csp(task);

        // Check if {1, 0} is applicable in some state
        csp.push();
        csp.addPreconditions();
        csp.assignActionVarSet({1, 0});
        CHECK(csp.hasSolution());
        csp.pop();

        // Check if {0, 1} is applicable in some state
        csp.push();
        csp.addPreconditions();
        csp.assignActionVarSet({0, 1});
        CHECK(csp.hasSolution());
        csp.pop();

        // Check if there is a state where both {1, 0} and {0, 1} are applicable
        csp.addActionVarSet();
        csp.addPreconditions(0);
        csp.addPreconditions(1);
        csp.assignActionVarSet({1, 0}, 0);
        csp.assignActionVarSet({0, 1}, 1);
        CHECK(!csp.hasSolution());
    }
}
} // namespace prost::parser
