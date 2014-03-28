#ifndef STATE_SET_GENERATOR_H
#define STATE_SET_GENERATOR_H

#include <vector>

class PlanningTask;
class State;

class StateSetGenerator {
public:
    StateSetGenerator(PlanningTask* _task) :
        task(_task) {}

    void generateStateSet(int const& numberOfStates = 200, double const& inclusionProb = 0.1, double const& maxTimeout = 2.0);

protected:
    PlanningTask* task;

    void applyRandomAction(State const& current, State& next, double& reward) const;
    int getRandomApplicableAction(State const& current) const;
};

#endif
