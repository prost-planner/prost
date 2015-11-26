#include "random_walk.h"

/******************************************************************
                     Search Engine Creation
******************************************************************/

RandomWalk::RandomWalk() :
    ProbabilisticSearchEngine("RandomWalk"),
    numberOfIterations(1) {}

bool RandomWalk::setValueFromString(std::string& param,
                                    std::string& value) {

    if (param == "-it") {
        setNumberOfIterations(atoi(value.c_str()));
        return true;
    }
    
    return SearchEngine::setValueFromString(param, value);
}

/******************************************************************
                       Main Search Functions
******************************************************************/

bool RandomWalk::estimateQValues(State const& _rootState,
                                 std::vector<int> const& actionsToExpand,
                                 std::vector<double>& qValues) {
    assert(_rootState.stepsToGo() > 0);
    PDState current(_rootState);
    for (unsigned int actionIndex = 0; actionIndex < qValues.size(); ++actionIndex) {
        if (actionsToExpand[actionIndex] == actionIndex) {
            performRandomWalks(current, actionIndex, qValues[actionIndex]);
            qValues[actionIndex] /= (double)_rootState.stepsToGo();
        }
    }
    return true;
}

void RandomWalk::performRandomWalks(PDState const& root, int firstActionIndex,
                                    double& result) const {
    result = 0.0;
    double reward = 0.0;
    PDState next;

    for (unsigned int i = 0; i < numberOfIterations; ++i) {
        PDState current(root.stepsToGo() -1);
        sampleSuccessorState(root, firstActionIndex, current, reward);
        result += reward;

        while (current.stepsToGo() > 0) {
            std::vector<int> applicableActions = getIndicesOfApplicableActions(current);
            next.reset(current.stepsToGo() - 1);

            sampleSuccessorState(current, applicableActions[std::rand() % applicableActions.size()], next, reward);
            result += reward;
            current = next;
        }
    }
    result /= (double)numberOfIterations;
}

void RandomWalk::sampleSuccessorState(PDState const& current, int const& actionIndex,
                                      PDState& next, double& reward) const {
    calcReward(current, actionIndex, reward);
    calcSuccessorState(current, actionIndex, next);
    for (unsigned int varIndex = 0; varIndex < State::numberOfProbabilisticStateFluents; ++varIndex) {
        next.sample(varIndex);
    }
}
