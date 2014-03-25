#ifndef STATE_SET_GENERATOR_H
#define STATE_SET_GENERATOR_H

#include <vector>

class PlanningTask;
class State;

class StateSetGenerator {
public:
    StateSetGenerator(PlanningTask* _task) :
        task(_task) {}

    std::vector<State> generateStateSet(State const& rootState, int const& numberOfStates = 200, double const& inclusionProb = 0.1);

protected:
    PlanningTask* task;
};

#endif
