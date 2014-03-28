#include "state_set_generator.h"

#include "planning_task.h"
#include "evaluatables.h"

#include "utils/timer.h"
#include "utils/system_utils.h"

#include <cstdlib>
#include <iostream>

using namespace std;

void StateSetGenerator::generateStateSet(int const& numberOfStates, double const& inclusionProb, double const& maxTimeout) {
    Timer t;

    State currentState(task->CPFs);
    task->trainingSet.insert(currentState);

    int remainingSteps = task->horizon;

    while((task->trainingSet.size() < numberOfStates) && (MathUtils::doubleIsSmaller(t(), maxTimeout))) {
        State nextState(task->CPFs.size());
        double reward = 0.0;
        applyRandomAction(currentState, nextState, reward);
        --remainingSteps;

        if(MathUtils::doubleIsEqual(*task->rewardCPF->domain.begin(), reward) || 
           MathUtils::doubleIsEqual(*task->rewardCPF->domain.rbegin(), reward) ||
           MathUtils::doubleIsSmallerOrEqual(MathUtils::generateRandomNumber(), inclusionProb)) {
            task->trainingSet.insert(nextState);
        }

        if(remainingSteps > 0) {
            currentState = State(nextState);
        } else {
            currentState = State(task->CPFs);
            remainingSteps = task->horizon;
        }
    }
}

void StateSetGenerator::applyRandomAction(State const& current, State& next, double& reward) const {
    ActionState& randomAction = task->actionStates[getRandomApplicableAction(current)];

    for(unsigned int i = 0; i < task->CPFs.size(); ++i) {
        task->CPFs[i]->formula->evaluate(next[i], current, randomAction);
    }

    task->rewardCPF->formula->evaluate(reward, current, randomAction);
}

int StateSetGenerator::getRandomApplicableAction(State const& current) const {
    vector<int> result;
    for(unsigned int actionIndex = 0; actionIndex < task->actionStates.size(); ++actionIndex) {
        ActionState& action = task->actionStates[actionIndex];
        bool applicable = true;
        for(unsigned int precondIndex = 0; precondIndex < action.relevantSACs.size(); ++precondIndex) {
            double res = 0.0;
            action.relevantSACs[precondIndex]->formula->evaluate(res, current, action);
            if(MathUtils::doubleIsEqual(res, 0.0)) {
                applicable = false;
                break;
            }
        }
        if(applicable) {
            result.push_back(actionIndex);
        }
    }
    return result[std::rand() % result.size()];
}


