#include "fdr_generation.h"

#include "csp.h"
#include "logical_expressions.h"
#include "mutex_detection.h"
#include "rddl.h"

#include <algorithm>

using namespace std;

void VarPartition::addVarByIndex(int index) {
    assert(indices.find(index) == indices.end());
    indices.insert(index);
}

vector<ActionFluent*> FDRGenerator::generateFDRVars(
    TaskMutexInfo const& mutexes, Simplifications& replacements) {
    // Partition the variables with the given the mutex information
    VarPartitioning partitioning = partitionVars(mutexes);

    CSP csp(task);
    csp.addPreconditions();
    vector<ActionFluent*> result;
    for (VarPartition const& partition : partitioning) {
        if (partition.size() == 1) {
            // There is only one action variable in this partition, so we can
            // keep that variable
            int id = *partition.begin();
            assert(id >= 0 && id < task->actionFluents.size());
            result.push_back(task->actionFluents[id]);
        } else {
            result.push_back(generateFDRVar(partition, csp, replacements));
        }
    }
    return result;
}

ActionFluent* FDRGenerator::generateFDRVar(VarPartition const& partition,
                                           CSP& csp,
                                           Simplifications& replacements) {
    string name = "FDR-action-var-" + to_string(numFDRActionVars);
    string typeName = "FDR-action-var-type-" + to_string(numFDRActionVars);
    Type* fdrVarType = task->addType(typeName);
    ActionFluent* fdrVar = new ActionFluent(name, fdrVarType);

    // Check if at least one of these action fluents must be set to true
    int value = 0;
    Z3Expressions& actionVars = csp.getActionVars();
    csp.push();
    for (int id : partition) {
        csp.addConstraint(actionVars[id] == 0);
    }
    if (csp.hasSolution()) {
        task->addObject(
            typeName, "none-of-those-"+ to_string(numFDRActionVars));
        ++value;
    }
    csp.pop();

    for (int id : partition) {
        assert(id >= 0 && id < task->actionFluents.size());
        ActionFluent* var = task->actionFluents[id];

        string name = var->fullName;
        replace(name.begin(), name.end(), ' ', '~');
        task->addObject(typeName, name);

        // Replace all occurences of (old) binary action variable var with (new)
        // FDR action variable fdrVar
        vector<LogicalExpression*> eq = {fdrVar, new NumericConstant(value)};
        replacements[var] = new EqualsExpression(eq);
        ++value;
    }
    ++numFDRActionVars;
    return fdrVar;
}

void addVarToPartition(
    int varIndex, VarPartition& partition, vector<bool>& served) {
    partition.addVarByIndex(varIndex);
    served[varIndex] = true;
}

VarPartitioning GreedyFDRGenerator::partitionVars(
    TaskMutexInfo const& mutexInfo) {
    VarPartitioning result;

    size_t numActionVars = task->actionFluents.size();
    vector<bool> served(numActionVars, false);
    for (size_t varIndex = 0; varIndex < numActionVars; ++varIndex) {
        if (served[varIndex]) {
            // This variable has already been added to a mutex group
            continue;
        }
        VarMutexInfo const& mutexes = mutexInfo[varIndex];

        VarPartition partition;
        addVarToPartition(varIndex, partition, served);

        // Greedily check variables that are mutex with the action variable
        // with index varIndex if they are mutex with all action variables
        // that have been added to the partition so far
        for (int otherVarIndex : mutexes) {
            if (served[otherVarIndex]) {
                continue;
            }
            assert(otherVarIndex > varIndex);
            VarMutexInfo const& otherMutexes = mutexInfo[otherVarIndex];
            if (includes(otherMutexes.begin(), otherMutexes.end(),
                         partition.begin(), partition.end())) {
                addVarToPartition(otherVarIndex, partition, served);
            }
        }
        result.addPartition(move(partition));
    }
    return result;
}