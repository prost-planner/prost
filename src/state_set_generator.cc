#include "state_set_generator.h"

#include "actions.h"

#include "utils/timer.h"
#include "utils/system_utils.h"

#include <cstdlib>

using namespace std;

StateSetGenerator::StateSetGenerator(ProstPlanner* _planner) :
    SearchEngine("StateSetGenerator", _planner, _planner->getProbabilisticTask(), resultDummy), 
    numberOfStates(200),
    inclusionProb(0.1),
    applicableActions(vector<double>(task->getNumberOfActions(),0.0)) {
}

void StateSetGenerator::_run() {
    set<State, State::CompareIgnoringRemainingSteps> res;
    Timer t;
    vector<int> actionsToExpandIndices;
    int chosenActionIndex = 0;
    double reward = 0.0;

    State nextState(task->getStateSize());
    nextState.reset(rootState.remainingSteps()-1);
    currentState.setTo(rootState);

    res.insert(currentState);

    while((res.size() < numberOfStates) && (t() < 2.0)) {
        task->setActionsToExpand(currentState, actionsToExpand);

        for(unsigned int i = 0; i < actionsToExpand.size(); ++i) {
            if(!MathUtils::doubleIsMinusInfinity(actionsToExpand[i])) {
                actionsToExpandIndices.push_back(i);
            }
        }
        chosenActionIndex = actionsToExpandIndices[rand() % actionsToExpandIndices.size()];
        task->calcStateTransition(currentState, chosenActionIndex, nextState, reward);

        if(task->isMinReward(reward) || task->isMaxReward(reward)) {
            res.insert(nextState);
        } else {
            generateRandomNumber(randNum);
            if(MathUtils::doubleIsSmallerOrEqual(randNum,inclusionProb)) {
                res.insert(nextState);
            }
        }

        if(nextState.isFinalState()) {
            nextState.reset(rootState.remainingSteps()-1);
            currentState.setTo(rootState);
        } else {
            currentState.reset(nextState.remainingSteps()-1);
            currentState.swap(nextState);
        }
    }

    for(set<State>::iterator it = res.begin(); it != res.end(); ++it) {
        stateSet.push_back(*it);
    }
    cout << "created " << stateSet.size() << " states." << endl;
}

inline void StateSetGenerator::generateRandomNumber(double& res) {
    res = ((double)(rand() % 1000001) / 1000000.0);
}
