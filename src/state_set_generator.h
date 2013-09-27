#ifndef STATE_SET_GENERATOR_H
#define STATE_SET_GENERATOR_H

#include <vector>

class ProstPlanner;
class PlanningTask;
class State;

class StateSetGenerator {
public:
    StateSetGenerator(ProstPlanner* _planner);

    std::vector<State> generateStateSet(State const& rootState, int const& numberOfStates = 200, double const& inclusionProb = 0.1);

protected:
    ProstPlanner* planner;
    PlanningTask* successorGenerator;
    PlanningTask* applicableActionGenerator;
};

#endif
