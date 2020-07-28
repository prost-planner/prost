#include "fdr_generation.h"

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
    vector<ActionFluent*> result;
    for (VarPartition const& partition : partitioning) {
        if (partition.size() == 1) {
            // There is only one action variable in this partition, so we can
            // keep that variable
            int id = *partition.begin();
            assert(id >= 0 && id < task->actionFluents.size());
            result.push_back(task->actionFluents[id]);
        } else {
            string name = "FDR-action-var-" + to_string(numFDRActionVars);
            string typeName =
                "FDR-action-var-type-" + to_string(numFDRActionVars);
            Type* fdrVarType = task->addType(typeName);
            ActionFluent* fdrVar = new ActionFluent(name, fdrVarType);

            // TODO: The implementation we had here isn't working, presumably
            //  because there are some parts of the code that treat the value
            //  '0' as a special value that corresponds to noop. We therefore
            //  had to comment the check if the 'none-of-those' value is
            //  required and now always create it. It would of course be better
            //  if the value is only created when it is actually needed. A good
            //  domain to test this is earth-observation-2018.

            // Check if at least one of these action fluents must be selected or
            // if there is also a "none-of-those" value
            int value = 0;
            // s.push();
            // for (int id : mutexGroup) {
            //     s.add(af_exprs[id] == 0);
            // }
            // if (s.check() == z3::sat) {
            task->addObject(typeName, "none-of-those-"+ to_string(numFDRActionVars));
            ++value;
            // }
            // s.pop();

            for (int id : partition) {
                assert(id >= 0 && id < task->actionFluents.size());
                ActionFluent* var = task->actionFluents[id];

                string name = var->fullName;
                replace(name.begin(), name.end(), ' ', '~');
                task->addObject(typeName, name);

                // Replace all occurences of (old) binary action fluent with (new)
                // FDR action fluent
                vector<LogicalExpression*> eq = {fdrVar, new NumericConstant(value)};
                replacements[var] = new EqualsExpression(eq);
                ++value;
            }
            result.push_back(fdrVar);
            ++numFDRActionVars;
        }
    }
    return result;
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