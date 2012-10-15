#include "state_set_generator.h"

#include "actions.h"

#include "utils/timer.h"
#include "utils/system_utils.h"

#include <cstdlib>

using namespace std;

StateSetGenerator::StateSetGenerator(ProstPlanner* _planner) :
    planner(_planner),
    task(planner->getProbabilisticTask()) {
}

vector<State> StateSetGenerator::generateStateSet(State const& rootState, int const& numberOfStates, double const& inclusionProb) {
    Timer t;

    set<State, State::CompareIgnoringRemainingSteps> stateSet;

    State nextState(task->getStateSize());
    nextState.reset(rootState.remainingSteps()-1);
    State currentState(rootState);

    stateSet.insert(currentState);

    while((stateSet.size() < numberOfStates) && (MathUtils::doubleIsSmaller(t(), 2.0))) {
        vector<int> actionsToExpand(task->getNumberOfActions(),0);
        task->setActionsToExpand(currentState, actionsToExpand);

        vector<int> actionsToExpandIndices;
        for(unsigned int i = 0; i < actionsToExpand.size(); ++i) {
            if(actionsToExpand[i] == i) {
                actionsToExpandIndices.push_back(i);
            }
        }

        int chosenActionIndex = actionsToExpandIndices[std::rand() % actionsToExpandIndices.size()];
        double reward = 0.0;
        task->calcStateTransition(currentState, chosenActionIndex, nextState, reward);

        if(task->isMinReward(reward) || task->isMaxReward(reward)) {
            stateSet.insert(nextState);
        } else {
            double randNum = 0.0;
            task->generateRandomNumber(randNum);
            if(MathUtils::doubleIsSmallerOrEqual(randNum,inclusionProb)) {
                stateSet.insert(nextState);
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

    vector<State> result(stateSet.size());
    std::copy(stateSet.begin(), stateSet.end(), result.begin());

    cout << "created " << result.size() << " states." << endl;
    return result;
}

