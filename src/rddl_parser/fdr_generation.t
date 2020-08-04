#ifndef FDR_GENERATION_T
#define FDR_GENERATION_T

#include "csp.h"
#include "logical_expressions.h"
#include "rddl.h"

namespace prost::parser::fdr {

template <typename PartitionGen>
std::vector<ActionFluent*> FDRGenerator::generateFDRVars(
    TaskMutexInfo const& mutexes, Simplifications& replacements,
    PartitionGen generator) {
    // Partition the variables with the given mutex information
    VarPartitioning partitioning = generator(mutexes);
    RDDLTaskCSP csp(task);
    csp.addPreconditions();
    std::vector<ActionFluent*> result;
    for (VarPartition const& partition : partitioning) {
        if (partition.size() == 1) {
            // There is only one action variable in this partition, so we
            // can keep that variable. Since a partitioning does not allow
            // to modify the pointer we take the original action fluent from
            // the task
            int const varIndex = (*partition.begin())->index;
            result.push_back(task->actionFluents[varIndex]);
        } else {
            result.push_back(generateFDRVar(partition, csp, replacements));
        }
    }
    return result;
}

} // namespace prost::parser::fdr

#endif
