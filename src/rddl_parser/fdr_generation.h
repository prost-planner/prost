#ifndef FDR_GENERATION_H
#define FDR_GENERATION_H

#include <map>
#include <set>
#include <vector>

namespace prost {
namespace parser {
class ActionFluent;
class LogicalExpression;
struct TaskMutexInfo;
class ParametrizedVariable;
struct RDDLTask;
class RDDLTaskCSP;

using Simplifications = std::map<ParametrizedVariable*, LogicalExpression*>;

/*
  A VarPartition object is used to represent a subset of the action variables
*/
struct VarPartition {
    VarPartition() = default;

    void addVar(ActionFluent* var);
    std::set<ActionFluent*>::const_iterator begin() const {
        return vars.begin();
    }
    std::set<ActionFluent*>::const_iterator end() const {
        return vars.end();
    }
    size_t size() const {
        return vars.size();
    }

private:
    struct ActionFluentSort {
        bool operator()(ActionFluent const* lhs, ActionFluent const* rhs) const;
    };
    std::set<ActionFluent*, ActionFluentSort> vars;
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
  An FDRGenerator generates finite-domain variables. Implement the method

  VarPartitioning partitionVars(TaskMutexInfo const& mutexInfo)

  that generates a partitioning of the variables given the provided mutex
  information.
*/
class FDRGenerator {
public:
    explicit FDRGenerator(RDDLTask* _task)
        : task(_task) {}

    std::vector<ActionFluent*> generateFDRVars(
        TaskMutexInfo const& mutexes, Simplifications& replacements);
    ActionFluent* generateFDRVar(VarPartition const& partition,
                                 RDDLTaskCSP& csp,
                                 Simplifications& replacements);

protected:
    RDDLTask* task;
    int numFDRActionVars = 0;

    virtual VarPartitioning partitionVars(TaskMutexInfo const& mutexInfo) = 0;
};

/*
  The GreedyFDRGenerator builds a partition by starting with the first variable
  that is mutex with some variable and by adding variables that are mutex with
  all variables that were already added. This process iterates until no
  variables are left.
*/
class GreedyFDRGenerator : public FDRGenerator {
public:
    explicit GreedyFDRGenerator(RDDLTask* _task) : FDRGenerator(_task) {}

protected:
    VarPartitioning partitionVars(TaskMutexInfo const& mutexInfo) override;
};
} // namespace parser
} // namespace prost

#endif
