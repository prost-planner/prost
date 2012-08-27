#ifndef STATE_SET_GENERATOR_H
#define STATE_SET_GENERATOR_H

#include "search_engine.h"
#include "prost_planner.h"

class StateSetGenerator;
class RandomSearch;

class StateSetGenerator : public SearchEngine {
public:
    StateSetGenerator(ProstPlanner* _planner);

    void setNumberOfStates(int _numberOfStates) {
        numberOfStates = _numberOfStates;
    }

    void setInclusionProbability(double _inclusionProb) {
        inclusionProb = _inclusionProb;
    }

    std::vector<State> const& getGeneratedStateSet() {
        return stateSet;
    }

protected:
    void _run();
    void generateRandomNumber(double& res);

    int numberOfStates;
    double inclusionProb;
    std::vector<State> stateSet;
    std::vector<double> applicableActions;
    double randNum;
    std::vector<double> resultDummy;
};

#endif
