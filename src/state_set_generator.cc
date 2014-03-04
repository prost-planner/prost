#include "state_set_generator.h"

#include "prost_planner.h"
#include "planning_task.h"

#include "utils/timer.h"
#include "utils/system_utils.h"

#include <cstdlib>

using namespace std;

StateSetGenerator::StateSetGenerator(ProstPlanner* _planner) :
    planner(_planner),
    successorGenerator(planner->getProbabilisticTask()) {

    if(successorGenerator->isPruningEquivalentToDeterminization()) {
        applicableActionGenerator = planner->getDeterministicTask();
    } else {
        applicableActionGenerator = planner->getProbabilisticTask();
    }
}

vector<State> StateSetGenerator::generateStateSet(State const& rootState, int const& numberOfStates, double const& inclusionProb) {
    Timer t;

    set<State, State::CompareIgnoringRemainingSteps> stateSet;

    State currentState(rootState);
    State nextState(successorGenerator->getStateSize(), -1, successorGenerator->getNumberOfStateFluentHashKeys());
    nextState.reset(rootState.remainingSteps()-1);
    
    stateSet.insert(currentState);
    while((stateSet.size() < numberOfStates) && (MathUtils::doubleIsSmaller(t(), 2.0))) {
        vector<int> actionsToExpandIndices = applicableActionGenerator->getIndicesOfApplicableActions(currentState);
        int chosenActionIndex = actionsToExpandIndices[std::rand() % actionsToExpandIndices.size()];

        double reward = 0.0;
        successorGenerator->sampleStateTransition(currentState, chosenActionIndex, nextState, reward);

        if(successorGenerator->isMinReward(reward) || successorGenerator->isMaxReward(reward) ||
           MathUtils::doubleIsSmallerOrEqual(MathUtils::generateRandomNumber(), inclusionProb)) {
            stateSet.insert(nextState);
        }
        
        if(nextState.isTerminal()) {
            nextState.reset(rootState.remainingSteps()-1);
            currentState.setTo(rootState);
        } else {
            currentState.reset(nextState.remainingSteps()-1);
            currentState.swap(nextState);
        }
    }

    vector<State> result(stateSet.size());
    std::copy(stateSet.begin(), stateSet.end(), result.begin());

    // for(unsigned int i = 0; i < result.size(); ++i) {
    //     successorGenerator->printState(cout, result[i]);
    //     cout << endl << endl;
    // }

    // assert(false);

    cout << "created " << result.size() << " states." << endl;
    return result;
}

