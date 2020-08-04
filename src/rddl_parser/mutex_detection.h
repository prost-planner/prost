#ifndef MUTEX_DETECTION_H
#define MUTEX_DETECTION_H

#include <set>
#include <vector>

namespace prost::parser {
class ActionFluent;
struct RDDLTask;
namespace fdr {

struct ActionFluentSort {
    bool operator()(ActionFluent const* lhs, ActionFluent const* rhs) const;
};

/*
  A VariableMutexInformation is associated with a (binary) action variable and
  keeps track of all other (binary) action variables it is mutex with (two
  binary action variables are considered mutex if all action states where both
  are assigned 'true' are inapplicable in all states).
*/
class VarMutexInfo {
public:
    explicit VarMutexInfo(RDDLTask* _task, ActionFluent const* _var)
        : task(_task), var(_var) {}

    void addAllVars();
    void addVar(ActionFluent const* other);
    bool isMutexWithSomeVar() const {
        return !mutexVars.empty();
    }
    bool isMutexWithAllVars() const;
    bool isMutexWith(ActionFluent const* other) const {
        return mutexVars.find(other) != mutexVars.end();
    }
    std::set<ActionFluent const*>::const_iterator begin() const {
        return mutexVars.begin();
    }
    std::set<ActionFluent const*>::const_iterator end() const {
        return mutexVars.end();
    }

private:
    RDDLTask* const task;
    ActionFluent const* var;

    std::set<ActionFluent const*, ActionFluentSort> mutexVars;
};

/*
  A TaskMutexInfo object maintains the mutex information of all action variables
  of a RDDL task
*/
class TaskMutexInfo {
public:
    explicit TaskMutexInfo(RDDLTask* task);

    void addMutexInfo(ActionFluent const* lhs, ActionFluent const* rhs);
    bool hasMutexVarPair() const;
    bool allVarsArePairwiseMutex() const;
    VarMutexInfo const& operator[](ActionFluent const* var) const;
    VarMutexInfo& operator[](ActionFluent const* var);
    std::vector<VarMutexInfo>::const_iterator begin() const {
        return mutexInfoOfVars.begin();
    }
    std::vector<VarMutexInfo>::const_iterator end() const {
        return mutexInfoOfVars.end();
    }
    size_t size() const {
        return mutexInfoOfVars.size();
    }

    RDDLTask const* getTask() const {
        return task;
    }

private:
    std::vector<VarMutexInfo> mutexInfoOfVars;
    RDDLTask* task;
};

/*
  Compute mutex information for each pair of action variables and return the
  information in a TaskMutexInfo object.
*/
TaskMutexInfo computeActionVarMutexes(RDDLTask* task);
} // namespace fdr
} // namespace prost::parser

#endif
