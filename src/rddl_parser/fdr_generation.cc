#include "fdr_generation.h"

#include "csp.h"
#include "logical_expressions.h"
#include "mutex_detection.h"
#include "rddl.h"

#include <algorithm>

using namespace std;

namespace prost::parser::fdr {

int FDRGenerator::numFDRActionVars = 0;

void VarPartition::addVar(ActionFluent const* var) {
    assert(vars.find(var) == vars.end());
    vars.insert(var);
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
    for (ActionFluent const* var : partition) {
        csp.addConstraint(actionVars[var->index] == 0);
    }
    // If there is a valid assignment where all action fluents are 0 we add
    // "none-of-those" as a domain value of this FDR variable
    if (csp.hasSolution()) {
        task->addObject(typeName,
                        "none-of-those-" + to_string(numFDRActionVars));
        ++value;
    }
    csp.pop();

    for (ActionFluent const* var : partition) {
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

void addVarToPartition(ActionFluent const* var, VarPartition& partition,
                       vector<bool>& served) {
    partition.addVar(var);
    served[var->index] = true;
}

VarPartitioning GreedyPartitioning::operator()(
    TaskMutexInfo const& mutexInfo) const {
    VarPartitioning result;

    vector<ActionFluent*> const& vars = mutexInfo.getTask()->actionFluents;
    vector<bool> served(vars.size(), false);
    for (ActionFluent const* var : vars) {
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
        for (ActionFluent const* other : mutexes) {
            if (served[other->index]) {
                continue;
            }
            assert(var->index < other->index);

            VarMutexInfo const& otherMutexes = mutexInfo[other];
            if (includes(otherMutexes.begin(), otherMutexes.end(),
                         partition.begin(), partition.end(),
                         ActionFluentSort())) {
                addVarToPartition(other, partition, served);
            }
        }
        result.addPartition(move(partition));
    }
    return result;
}
} // namespace prost::parser::fdr
