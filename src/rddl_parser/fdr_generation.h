#ifndef FDR_GENERATION_H
#define FDR_GENERATION_H

#include <vector>

class ActionFluent;
class CSP;
class LogicalExpression;
struct TaskMutexInfo;
class ParametrizedVariable;
struct RDDLTask;

#include <map>
#include <set>

using Simplifications = std::map<ParametrizedVariable*, LogicalExpression*>;

/*
  A VarPartition object is used to represent a subset of the action variables
*/
struct VarPartition {
    VarPartition() {}

    void addVarByIndex(int index);

    std::set<int>::const_iterator begin() const {
        return indices.begin();
    }

    std::set<int>::const_iterator end() const {
        return indices.end();
    }

    size_t size() const {
        return indices.size();
    }

private:
    std::set<int> indices;
};

/*
  A VarPartitioning object is used to encode a partitioning of the action
  variables
*/
struct VarPartitioning {
    VarPartitioning() {}

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
  FDRGenerator is the pure virtual base class for algorithms that generate
  finite-domain variables. Implement the method

  VarPartitioning partitionVars(TaskMutexInfo const& mutexInfo)

  that generates a partitioning of the variables given the provided mutex
  information.
*/
class FDRGenerator {
public:
    explicit FDRGenerator(RDDLTask* _task)
        : task(_task), numFDRActionVars(0) {}

    std::vector<ActionFluent*> generateFDRVars(
        TaskMutexInfo const& mutexes, Simplifications& replacements);
    ActionFluent* generateFDRVar(
        VarPartition const& partition, CSP& csp, Simplifications& replacements);

protected:
    RDDLTask* task;
    int numFDRActionVars;

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
    GreedyFDRGenerator() = delete;
    explicit GreedyFDRGenerator(RDDLTask* _task)
        : FDRGenerator(_task) {}

protected:
    VarPartitioning partitionVars(TaskMutexInfo const& mutexInfo) override;
};

#endif
