#include "csp.h"

#include "evaluatables.h"
#include "rddl.h"

using namespace std;

namespace prost {
namespace parser {
RDDLTaskCSP::RDDLTaskCSP(RDDLTask* _task)
    : task(_task), context(), solver(context) {
    addStateVarSet();
    addActionVarSet();
}

template <typename T>
::z3::expr createZ3Var(::z3::context& context, T const* var, int setIndex) {
    string name(to_string(var->index) + "_" + var->fullName + "_" +
                to_string(setIndex));
    return context.int_const(name.c_str());
}

void RDDLTaskCSP::addStateVarSet() {
    Z3Expressions stateVars;
    int setIndex = static_cast<int>(stateVarSets.size());
    for (ConditionalProbabilityFunction const* cpf : task->CPFs) {
        StateFluent const* var = cpf->head;
        stateVars.emplace_back(createZ3Var(context, var, setIndex));
        solver.add(stateVars.back() >= 0);
        solver.add(stateVars.back() < var->domainSize);
    }
    stateVarSets.emplace_back(stateVars);
}

void RDDLTaskCSP::addActionVarSet() {
    Z3Expressions actionVars;
    int setIndex = static_cast<int>(actionVarSets.size());
    for (ActionFluent const* var : task->actionFluents) {
        actionVars.emplace_back(createZ3Var(context, var, setIndex));
        solver.add(actionVars.back() >= 0);
        solver.add(actionVars.back() < static_cast<int>(var->domainSize()));
    }
    actionVarSets.emplace_back(actionVars);
}

void RDDLTaskCSP::addPreconditions(int actionSetIndex) {
    assert(static_cast<size_t>(actionSetIndex) < actionVarSets.size());
    for (ActionPrecondition const* precond : task->preconds) {
        solver.add(precond->formula->toZ3Formula(*this, actionSetIndex) != 0);
    }

    addConcurrencyConstraint(actionSetIndex);
}

void RDDLTaskCSP::addConcurrencyConstraint(int actionSetIndex) {
    int numActionFluents = task->actionFluents.size();
    int numConcurrentActions = task->numberOfConcurrentActions;
    if (numConcurrentActions < numActionFluents) {
        vector<LogicalExpression*> vars(numActionFluents);
        copy(task->actionFluents.begin(), task->actionFluents.end(),
             vars.begin());
        vector<LogicalExpression*> maxConcurrent =
            {new Addition(vars), new NumericConstant(numConcurrentActions)};
        auto* constraint = new LowerEqualsExpression(maxConcurrent);
        solver.add(constraint->toZ3Formula(*this, actionSetIndex) != 0);
    }
}

void RDDLTaskCSP::assignActionVarSet(
    vector<int> const& values, int actionSetIndex) {
    assert(static_cast<size_t>(actionSetIndex) < actionVarSets.size());
    Z3Expressions const& actionVars = actionVarSets[actionSetIndex];
    assert(actionVars.size() == values.size());
    for (size_t i = 0; i < values.size(); ++i) {
        solver.add(actionVars[i] == values[i]);
    }
}

vector<int> RDDLTaskCSP::getActionModel(int actionSetIndex) const {
    assert(static_cast<size_t>(actionSetIndex) < actionVarSets.size());
    Z3Expressions const& action = actionVarSets[actionSetIndex];
    int numActionFluents = task->actionFluents.size();
    vector<int> result(numActionFluents);
    ::z3::model model = solver.get_model();
    for (size_t index = 0; index < numActionFluents; ++index) {
        ::z3::expr const& actionFluent = action[index];
        // The internal representation of numbers in z3 does not use ints, so
        // the conversion is non-trivial. A recommended way is to convert to a
        // string and from there to an int.
        int value = atoi(model.eval(actionFluent).to_string().c_str());
        result[index] = value;
    }
    return result;
}

void RDDLTaskCSP::invalidateActionModel(int actionSetIndex) {
    assert(static_cast<size_t>(actionSetIndex) < actionVarSets.size());
    Z3Expressions const& action = actionVarSets[actionSetIndex];
    ::z3::model model = solver.get_model();
    ::z3::expr block = context.bool_val(false);
    for (::z3::expr const& actionFluent : action) {
        int value = atoi(model.eval(actionFluent).to_string().c_str());
        block = block || (actionFluent != value);
    }
    solver.add(block);
}
} // namespace parser
} // namespace prost
