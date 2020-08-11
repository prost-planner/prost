#include "reachability_analysis.h"

#include "evaluatables.h"
#include "rddl.h"

#include <algorithm>

using namespace std;

namespace prost::parser {
MinkowskiReachabilityAnalyser::MinkowskiReachabilityAnalyser(RDDLTask* _task)
    : ReachabilityAnalyser(_task),
      step(0) {
    prepareActionEquivalence();
}

void MinkowskiReachabilityAnalyser::prepareActionEquivalence() {
    for (ConditionalProbabilityFunction* cpf : task->CPFs) {
        auto compFn = [&] (int i, int j) {
            ActionState const& lhs = task->actionStates[i];
            ActionState const& rhs = task->actionStates[j];
            for (ActionFluent* af : cpf->dependentActionFluents) {
                if (lhs[af->index] > rhs[af->index]) {
                    return true;
                }
            }
            return false;
        };
        // We use a set to only add one representative action for each
        // equivalence class, and then transform it to a vector
        set<int, decltype(compFn)> representatives(compFn);
        if (cpf->isActionIndependent()) {
            representatives.insert(0);
        } else {
            for (ActionState const& action : task->actionStates) {
                representatives.insert(action.index);
            }
        }
        actionIndicesByCPF.emplace_back(
            representatives.begin(), representatives.end());
    }
}

vector<set<double>> MinkowskiReachabilityAnalyser::determineReachableFacts() {
    size_t numCPFs = task->CPFs.size();
    // Start fixed point iteration with initial values
    vector<set<double>> domains(numCPFs);
    for (size_t i = 0; i < numCPFs; ++i) {
        domains[i].insert(task->CPFs[i]->getInitialValue());
    }

    step = 0;
    bool fixedPointReached = false;
    while ((step < task->horizon) && !fixedPointReached) {
        ++step;
        fixedPointReached = true;
        vector<set<double>> reached = performStep(domains);
        // Add nextValues to domains and check if a fixed point has been reached
        for (size_t i = 0; i < numCPFs; ++i) {
            if (!includes(domains[i].begin(), domains[i].end(),
                          reached[i].begin(), reached[i].end())) {
                domains[i].insert(reached[i].begin(), reached[i].end());
                fixedPointReached = false;
            }
        }
    }
    return domains;
}

vector<set<double>> MinkowskiReachabilityAnalyser::performStep(
    vector<set<double>>& values) const {
    // Check each CPF if additional values can be derived
    vector<set<double>> result(task->CPFs.size());
    for (ConditionalProbabilityFunction* cpf : task->CPFs) {
        if (values[cpf->head->index].size() == cpf->getDomainSize()) {
            continue;
        }
        result[cpf->head->index] = applyRepresentativeActions(cpf, values);
    }
    return result;
}

set<double> MinkowskiReachabilityAnalyser::applyRepresentativeActions(
    ConditionalProbabilityFunction* cpf,
    vector<set<double>> const& values) const {
    // Apply all representative actions associated with this CPF
    set<double> result;
    for (int actionIndex : actionIndicesByCPF[cpf->head->index]) {
        set<double> reachedByAction;
        cpf->formula->calculateDomain(
            values, task->actionStates[actionIndex], reachedByAction);
        assert(!reachedByAction.empty());
        result.insert(reachedByAction.begin(), reachedByAction.end());
    }
    return result;
}
} // namespace prost::parser
