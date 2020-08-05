#ifndef MUTEX_DETECTION_H
#define MUTEX_DETECTION_H

#include <set>
#include <vector>

namespace prost {
namespace parser {
class ActionFluent;
struct RDDLTask;

/*
  A VariableMutexInformation is associated with a (binary) action variable and
  keeps track of all other (binary) action variables it is mutex with (two
  binary action variables are considered mutex if all action states where both
  are assigned 'true' are inapplicable in all states).
*/
class VarMutexInfo {
public:
    explicit VarMutexInfo(RDDLTask* _task, ActionFluent* _var)
        : task(_task), var(_var) {}

    void addAllVars();
    void addVar(ActionFluent* other);
    bool isMutexWithSomeVar() const {
        return !mutexVars.empty();
    }
    bool isMutexWithAllVars() const;
    bool isMutexWith(ActionFluent* other) const {
        return mutexVars.find(other) != mutexVars.end();
    }
    std::set<ActionFluent*>::const_iterator begin() const {
        return mutexVars.begin();
    }
    std::set<ActionFluent*>::const_iterator end() const {
        return mutexVars.end();
    }

private:
    RDDLTask* const task;
    ActionFluent* const var;

    struct ActionFluentSort {
        bool operator()(ActionFluent const* lhs, ActionFluent const* rhs) const;
    };
    std::set<ActionFluent*, ActionFluentSort> mutexVars;
};

/*
  A TaskMutexInfo object maintains the mutex information of all action variables
  of a RDDL task
*/
class TaskMutexInfo {
public:
    explicit TaskMutexInfo(RDDLTask* task);

    void addMutexInfo(ActionFluent* lhs, ActionFluent* rhs);
    bool hasMutexVarPair() const;
    bool allVarsArePairwiseMutex() const;
    VarMutexInfo const& operator[](ActionFluent* var) const;
    VarMutexInfo& operator[](ActionFluent* var);
    std::vector<VarMutexInfo>::const_iterator begin() const {
        return mutexInfoOfVars.begin();
    }
    std::vector<VarMutexInfo>::const_iterator end() const {
        return mutexInfoOfVars.end();
    }
    size_t size() const {
        return mutexInfoOfVars.size();
    }

private:
    std::vector<VarMutexInfo> mutexInfoOfVars;
};

/*
  Compute mutex information for each pair of action variables and return the
  information in a TaskMutexInfo object.
*/
TaskMutexInfo computeActionVarMutexes(RDDLTask* task);
} // namespace parser
} // namespace prost

#endif
