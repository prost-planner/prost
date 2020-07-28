#ifndef MUTEX_DETECTION_H
#define MUTEX_DETECTION_H

#include <set>
#include <vector>

struct RDDLTask;

/*
  A VariableMutexInformation object allows to maintain all mutex information of
  a single action variable
*/
struct VarMutexInfo {
    VarMutexInfo() = delete;
    explicit VarMutexInfo(RDDLTask* _task, int _varIndex)
        : task(_task), varIndex(_varIndex) {}

    void addAllVars();
    void addVarByIndex(int index);

    bool hasMutex() const {
        return !mutex.empty();
    }
    bool isMutexWithAllVars() const;
    bool isMutexWith(int otherVarIndex) const {
        return mutex.find(otherVarIndex) != mutex.end();
    }

    size_t size() const {
        return mutex.size();
    }

    std::set<int>::const_iterator begin() const {
        return mutex.begin();
    }

    std::set<int>::const_iterator end() const {
        return mutex.end();
    }
private:
    RDDLTask* const task;
    int const varIndex;
    std::set<int> mutex;
};

/*
  A TaskMutexInfo object maintains the mutex information of all action variables
  of a RDDL task
*/
struct TaskMutexInfo {
    explicit TaskMutexInfo(RDDLTask* task);

    std::vector<VarMutexInfo> mutexByVar;

    void varsAreMutex(int lhs, int rhs) {
        mutexByVar[lhs].addVarByIndex(rhs);
        mutexByVar[rhs].addVarByIndex(lhs);
    }

    bool hasMutex() const;
    bool allVarsArePairwiseMutex() const;

    size_t size() const {
        return mutexByVar.size();
    }

    VarMutexInfo const& operator[](int index) const {
        return mutexByVar[index];
    }

    VarMutexInfo& operator[](int index) {
        return mutexByVar[index];
    }

    std::vector<VarMutexInfo>::const_iterator begin() const {
        return mutexByVar.begin();
    }

    std::vector<VarMutexInfo>::const_iterator end() const {
        return mutexByVar.end();
    }
};

/*
  Compute mutex information for each pair of action variables and return the
  information in a TaskMutexInfo object.
*/
extern TaskMutexInfo computeActionVarMutexes(RDDLTask* task);

#endif
