#include "csp.h"

#include "evaluatables.h"
#include "rddl.h"

#include <sstream>

using namespace std;

CSP::CSP(RDDLTask* _task) : task(_task), context(), solver(context) {
    stringstream ss;
    for (ConditionalProbabilityFunction* cpf : task->CPFs) {
        ss.str("");
        StateFluent* sf = cpf->head;
        ss << "sf_" << sf->index << "_" << sf->fullName;
        state.push_back(context.int_const(ss.str().c_str()));
        solver.add(state.back() >= 0);
        int domainSize = sf->valueType->objects.size();
        solver.add(state.back() < domainSize);
    }

    addActionVariables();
}

// Add (another) set of action variables
void CSP::addActionVariables() {
    Z3Expressions action;
    int index = actions.size();
    stringstream ss;
    for (ActionFluent* af : task->actionFluents) {
        ss.str("");
        ss << "af_" << af->index << "_" << af->fullName << "_" << index;
        action.push_back(context.int_const(ss.str().c_str()));
        solver.add(action.back() >= 0);
        int domainSize = af->valueType->objects.size();
        solver.add(action.back() < domainSize);
    }
    actions.push_back(action);
}

// Add all preconditions for the specified action variables
void CSP::addPreconditions(int actionIndex) {
    assert(actionIndex < actions.size());
    for (LogicalExpression* sac : task->SACs) {
        solver.add(sac->toZ3Formula(*this, actionIndex) != 0);
    }

    // The value provided by max-nondef-actions is also a constraint that
    // must be taken into account.
    int numActionFluents = task->actionFluents.size();
    int numConcurrentActions = task->numberOfConcurrentActions;
    if (numConcurrentActions < numActionFluents) {
        vector<LogicalExpression*> afs;
        for (ActionFluent* af : task->actionFluents) {
            afs.push_back(af);
        }
        vector<LogicalExpression*> maxConcurrent =
            {new Addition(afs), new NumericConstant(numConcurrentActions)};
        LowerEqualsExpression* constraint =
            new LowerEqualsExpression(maxConcurrent);
        solver.add(constraint->toZ3Formula(*this, actionIndex) != 0);
    }
}

// Assign the specified action variables to the given values
void CSP::assignActionVariables(
    std::vector<int> const& values, int actionIndex) {
    assert(actionIndex < actions.size());
    Z3Expressions const& action = actions[actionIndex];
    for (size_t i = 0; i < values.size(); ++i) {
        solver.add(action[i] == values[i]);
    }
}

// Invalidate the solution for the specified action variables
void CSP::invalidateActionModel(int actionIndex) {
    assert(actionIndex < actions.size());
    Z3Expressions const& action = actions[actionIndex];
    z3::model model = solver.get_model();
    z3::expr block = context.bool_val(false);
    for (z3::expr const& actionFluent : action) {
        int value = atoi(model.eval(actionFluent).to_string().c_str());
        block = block || (actionFluent != value);
    }
    solver.add(block);
}

// Return the solution for the specified action variables
std::vector<int> CSP::getActionModel(int actionIndex) const {
    assert(actionIndex < actions.size());
    Z3Expressions const& action = actions[actionIndex];
    int numActionFluents = task->actionFluents.size();
    std::vector<int> result(numActionFluents);
    z3::model model = solver.get_model();
    for (size_t index = 0; index < numActionFluents; ++index) {
        z3::expr const& actionFluent = action[index];
        // The internal representation of numbers in Z3 does not use ints,
        // so the conversion is non-trivial. A way that is recommended is to
        // convert to a string and from there to an int.
        int value = atoi(model.eval(actionFluent).to_string().c_str());
        result[index] = value;
    }
    return move(result);
}