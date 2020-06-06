#ifndef PRECOMPUTER_H
#define PRECOMPUTER_H

class Evaluatable;
class RDDLTask;
class State;
class StateFluent;

#include <vector>

class Precomputer {
public:
    Precomputer(RDDLTask* task)
        : task(task) {}

    void precompute();

private:
    RDDLTask* task;

    void precomputeEvaluatable(Evaluatable* eval);
    void createRelevantStates(std::vector<StateFluent*>& dependentStateFluents,
                              std::vector<State>& result);
    long calculateStateFluentHashKey(Evaluatable* eval,
                                     State const& state) const;
};

#endif
