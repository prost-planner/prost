#include "fdr_generation.h"

#include "csp.h"
#include "logical_expressions.h"
#include "mutex_detection.h"
#include "rddl.h"

#include <algorithm>

using namespace std;

namespace prost {
namespace parser {
inline bool VarPartition::ActionFluentSort::operator()(ActionFluent const* lhs, ActionFluent const* rhs) const {
    return lhs->index < rhs->index;
}

void VarPartition::addVar(ActionFluent* var) {
    assert(vars.find(var) == vars.end());
    vars.insert(var);
}

vector<ActionFluent*> FDRGenerator::generateFDRVars(
    TaskMutexInfo const& mutexes, Simplifications& replacements) {
    // Partition the variables with the given mutex information
    VarPartitioning partitioning = partitionVars(mutexes);
    RDDLTaskCSP csp(task);
    csp.addPreconditions();
    vector<ActionFluent*> result;
    for (VarPartition const& partition : partitioning) {
        if (partition.size() == 1) {
            // There is only one action variable in this partition, so we can
            // keep that variable
            result.push_back(*partition.begin());
        } else {
            result.push_back(generateFDRVar(partition, csp, replacements));
        }
    }
    return result;
}

ActionFluent* FDRGenerator::generateFDRVar(VarPartition const& partition,
                                           RDDLTaskCSP& csp,
                                           Simplifications& replacements) {
    string name = "FDR-action-var-" + to_string(numFDRActionVars);
    string typeName = "FDR-action-var-type-" + to_string(numFDRActionVars);
    Type* fdrVarType = task->addType(typeName);
    ActionFluent* fdrVar = new ActionFluent(name, fdrVarType);

    // Check if at least one of these action fluents must be set to true
    int value = 0;
    Z3Expressions const& actionVars = csp.getActionVarSet();
    csp.push();
    for (ActionFluent* var : partition) {
        csp.addConstraint(actionVars[var->index] == 0);
    }
    if (csp.hasSolution()) {
        task->addObject(
            typeName, "none-of-those-"+ to_string(numFDRActionVars));
        ++value;
    }
    csp.pop();

    for (ActionFluent* var : partition) {
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
    ActionFluent* var, VarPartition& partition, vector<bool>& served) {
    partition.addVar(var);
    served[var->index] = true;
}

VarPartitioning GreedyFDRGenerator::partitionVars(
    TaskMutexInfo const& mutexInfo) {
    VarPartitioning result;

    // std::includes below expects the sets it operates on to be sorted. Since
    // we use a custom comparator to order elements in VarPartition and
    // VarMutexInfo, we need to make sure we consider the same order here.
    auto comp = [] (ActionFluent* lhs, ActionFluent* rhs) {return lhs->index < rhs->index;};

    size_t numActionVars = task->actionFluents.size();
    vector<bool> served(numActionVars, false);
    for (ActionFluent* var : task->actionFluents) {
        if (served[var->index]) {
            // This variable has already been added to a mutex group
            continue;
        }
        VarMutexInfo const& mutexes = mutexInfo[var];
        VarPartition partition;
        addVarToPartition(var, partition, served);

        // Greedily check variables that are mutex with the action variable
        // with index varIndex if they are mutex with all action variables
        // that have been added to the partition so far
        for (ActionFluent* other : mutexes) {
            if (served[other->index]) {
                continue;
            }
            assert(var->index < other->index);
            VarMutexInfo const& otherMutexes = mutexInfo[other];
            if (includes(otherMutexes.begin(), otherMutexes.end(),
                         partition.begin(), partition.end(), comp)) {
                addVarToPartition(other, partition, served);
            }
        }
        result.addPartition(move(partition));
    }
    return result;
}
} // namespace parser
} // namespace prost
