#include "mutex_detection.h"

#include "csp.h"
#include "logical_expressions.h"
#include "rddl.h"

#include <algorithm>

using namespace std;

void VarMutexInfo::addAllVars() {
    for (int index = 0; index < task->actionFluents.size(); ++index) {
        addVarByIndex(index);
    }
}

void VarMutexInfo::addVarByIndex(int index) {
    assert(index >= 0);
    assert(index < task->actionFluents.size());
    if (index != varIndex) {
        mutex.insert(index);
    }
}

bool VarMutexInfo::isMutexWithAllVars() const {
    return size() == (task->actionFluents.size() - 1);
}

TaskMutexInfo::TaskMutexInfo(RDDLTask* task) {
    for (int index = 0; index < task->actionFluents.size(); ++index) {
        mutexByVar.push_back(VarMutexInfo(task, index));
    }
}

bool TaskMutexInfo::hasMutex() const {
    auto checkFluent = [](VarMutexInfo const& m) {return m.hasMutex();};
    return any_of(mutexByVar.begin(), mutexByVar.end(), checkFluent);
}

bool TaskMutexInfo::allVarsArePairwiseMutex() const {
    auto checkFluent =
        [](VarMutexInfo const& m) {return m.isMutexWithAllVars();};
    return all_of(mutexByVar.begin(), mutexByVar.end(), checkFluent);
}

TaskMutexInfo computeActionVarMutexes(RDDLTask* task) {
    TaskMutexInfo result(task);
    // If there is only one action fluent (left) or if max-nondef-actions
    // doesn't constrain action applicability and there are no other
    // preconditions, no pair of action fluents can be mutex
    size_t numActionVars = task->actionFluents.size();
    bool concurrent = (task->numberOfConcurrentActions > 1);
    if ((numActionVars == 1) || (concurrent && task->SACs.empty())) {
        return result;
    }

    if (concurrent) {
        // TODO: Order the action variable pairs in a way that the most
        //  promising ones are considered earlier, and such that we can stop
        //  this if it takes too much time (the code below is quadratic in the
        //  number of action variables)

        CSP csp(task);
        csp.addPreconditions();
        Z3Expressions& actionVars = csp.getActionVars();

        for (ActionFluent* var : task->actionFluents) {
            // Action variables that are already in FDR are not considered
            if (var->isFDR) {
                continue;
            }
            int varIndex = var->index;
            for (ActionFluent* otherVar : task->actionFluents) {
                int otherVarIndex = otherVar->index;
                if ((otherVarIndex <= varIndex) || otherVar->isFDR) {
                    continue;
                }
                // Check if the CSP has a solution where both action variables
                // are true. If it hasn't, the action variables are mutex.
                csp.push();
                csp.addConstraint(actionVars[varIndex] == 1);
                csp.addConstraint(actionVars[otherVarIndex] == 1);
                if (!csp.hasSolution()) {
                    result.varsAreMutex(varIndex, otherVarIndex);
                }
                csp.pop();
            }
        }
    } else {
        // When there is no concurreny, all action variables are pairwise mutex
        // and they can be combined to a single FDR action variable
        for (size_t index = 0; index < numActionVars; ++index) {
            result[index].addAllVars();
        }
    }
    return result;
}