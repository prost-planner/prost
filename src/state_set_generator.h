#ifndef STATE_SET_GENERATOR_H
#define STATE_SET_GENERATOR_H

#include "prost_planner.h"

class StateSetGenerator;
class RandomSearch;

class StateSetGenerator {
public:
    StateSetGenerator(ProstPlanner* _planner);

    std::vector<State> generateStateSet(State const& rootState, int const& numberOfStates = 200, double const& inclusionProb = 0.1);

protected:
    ProstPlanner* planner;
    PlanningTask* task;
};

#endif
