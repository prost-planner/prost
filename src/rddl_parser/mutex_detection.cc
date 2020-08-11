#include "mutex_detection.h"

#include "csp.h"
#include "logical_expressions.h"
#include "rddl.h"

#include <algorithm>

using namespace std;

namespace prost::parser::fdr {
inline bool ActionFluentSort::operator()(ActionFluent const* lhs,
                                         ActionFluent const* rhs) const {
    return lhs->index < rhs->index;
}

void VarMutexInfo::addAllVars() {
    for (ActionFluent const* af : task->actionFluents) {
        addVar(af);
    }
}

void VarMutexInfo::addVar(ActionFluent const* other) {
    if (other != var) {
        mutexVars.insert(other);
    }
}

bool VarMutexInfo::isMutexWithAllVars() const {
    return mutexVars.size() == (task->actionFluents.size() - 1);
}

TaskMutexInfo::TaskMutexInfo(RDDLTask* _task) : task(_task) {
    for (ActionFluent const* af : task->actionFluents) {
        mutexInfoOfVars.push_back(VarMutexInfo(task, af));
    }
}

void TaskMutexInfo::addMutexInfo(ActionFluent const* lhs,
                                 ActionFluent const* rhs) {
    mutexInfoOfVars[lhs->index].addVar(rhs);
    mutexInfoOfVars[rhs->index].addVar(lhs);
}

bool TaskMutexInfo::hasMutexVarPair() const {
    auto isMutexWithSomeVar = [](VarMutexInfo const& m) {
        return m.isMutexWithSomeVar();
    };
    return any_of(mutexInfoOfVars.begin(), mutexInfoOfVars.end(),
                  isMutexWithSomeVar);
}

bool TaskMutexInfo::allVarsArePairwiseMutex() const {
    auto isMutexWithAllVars = [](VarMutexInfo const& m) {
        return m.isMutexWithAllVars();
    };
    return all_of(mutexInfoOfVars.begin(), mutexInfoOfVars.end(),
                  isMutexWithAllVars);
}

VarMutexInfo const& TaskMutexInfo::operator[](ActionFluent const* var) const {
    return mutexInfoOfVars[var->index];
}

VarMutexInfo& TaskMutexInfo::operator[](ActionFluent const* var) {
    return mutexInfoOfVars[var->index];
}

TaskMutexInfo computeActionVarMutexes(RDDLTask* task) {
    TaskMutexInfo result(task);
    // If there is only one action fluent (left) or if max-nondef-actions
    // doesn't constrain action applicability and there are no other
    // preconditions, no pair of action fluents can be mutex
    size_t const numActionVars = task->actionFluents.size();
    bool concurrent = (task->numberOfConcurrentActions > 1);
    if ((numActionVars == 1) || (concurrent && task->preconds.empty())) {
        return result;
    }

    if (concurrent) {
        RDDLTaskCSP csp(task);
        csp.addPreconditions();
        Z3Expressions const& actionVars = csp.getActionVarSet();

        for (ActionFluent const* var : task->actionFluents) {
            // Action variables that are already in FDR are not considered since
            // it can be expected that it will rarely be the case that FDR
            // variables are mutex with another variable in a later iteration
            // than when they were created in the first place. It might still
            // be worth to look into this at some point.
            if (var->isFDR) {
                continue;
            }
            for (ActionFluent const* other : task->actionFluents) {
                if ((other->index <= var->index) || other->isFDR) {
                    continue;
                }
                // Check if the CSP has a solution where both action variables
                // are true. If it hasn't, the action variables are mutex.
                csp.push();
                csp.addConstraint(actionVars[var->index] == 1);
                csp.addConstraint(actionVars[other->index] == 1);
                if (!csp.hasSolution()) {
                    result.addMutexInfo(var, other);
                }
                csp.pop();
            }
        }
    } else {
        // When there is no concurreny, all action variables are pairwise mutex
        // and they can be combined to a single FDR action variable
        for (ActionFluent const* var : task->actionFluents) {
            result[var].addAllVars();
        }
    }
    return result;
}
} // namespace prost::parser::fdr
