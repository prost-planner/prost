#ifndef PARSER_FDR_FDR_GENERATION_H
#define PARSER_FDR_FDR_GENERATION_H

#include "logical_expressions.h"

#include <map>
#include <set>
#include <vector>

namespace prost::parser {
class RDDLTaskCSP;
class ActionFluent;
class LogicalExpression;
class ParametrizedVariable;
struct RDDLTask;
namespace fdr {
class TaskMutexInfo;

using Simplifications =
    std::map<ParametrizedVariable const*, LogicalExpression*>;

/*
  A VarPartition object is used to represent a subset of the action variables
*/
struct VarPartition {
    VarPartition() = default;

    void addVar(ActionFluent const* var);
    std::set<ActionFluent const*>::const_iterator begin() const {
        return vars.begin();
    }
    std::set<ActionFluent const*>::const_iterator end() const {
        return vars.end();
    }
    size_t size() const {
        return vars.size();
    }

private:
    std::set<ActionFluent const*, ActionFluent::ActionFluentSort> vars;
};

/*
  A VarPartitioning object is used to encode a partitioning of the action
  variables
*/
struct VarPartitioning {
    VarPartitioning() = default;

    void addPartition(VarPartition&& partition) {
        partitioning.push_back(std::move(partition));
    }
    std::vector<VarPartition>::const_iterator begin() const {
        return partitioning.begin();
    }
    std::vector<VarPartition>::const_iterator end() const {
        return partitioning.end();
    }

private:
    std::vector<VarPartition> partitioning;
};

/*
  An FDRGenerator takes a generator that partitions variables according to mutex
  information and generates finite-domain variables from that partitioning.
*/
class FDRGenerator {
public:
    explicit FDRGenerator(RDDLTask* _task) : task(_task) {}

    template <typename PartitionGen>
    std::vector<ActionFluent*> generateFDRVars(TaskMutexInfo const& mutexes,
                                               Simplifications& replacements,
                                               PartitionGen generator);

private:
    ActionFluent* generateFDRVar(VarPartition const& partition,
                                 RDDLTaskCSP& csp,
                                 Simplifications& replacements);

    RDDLTask* task;

    // A counter on how many FDR variables have been generated yet
    static int numFDRActionVars;
};

/*
  GreedyPartitioning builds a partition by starting with the first variable
  that is mutex with some variable and by adding variables that are mutex with
  all variables that were already added. This process iterates until no
  variables are left.
*/
struct GreedyPartitioning {
    VarPartitioning operator()(TaskMutexInfo const& mutexes) const;
};
} // namespace fdr
} // namespace prost::parser

#include "fdr_generation.t"

#endif // PARSER_FDR_FDR_GENERATION_H
