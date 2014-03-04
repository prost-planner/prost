#include "depth_first_search.h"

#include "prost_planner.h"
#include "planning_task.h"

#include "utils/math_utils.h"

#include <iostream>
#include <set>

using namespace std;

/******************************************************************
                     Search Engine Creation
******************************************************************/

DepthFirstSearch::DepthFirstSearch(ProstPlanner* _planner) :
    SearchEngine("DFS", _planner, false),
    rewardHelperVar(0.0) {}

/******************************************************************
                       Main Search Functions
******************************************************************/

bool DepthFirstSearch::estimateQValues(State const& _rootState, vector<int> const& actionsToExpand, vector<double>& qValues) {
    assert(_rootState.remainingSteps() > 0);
    assert(_rootState.remainingSteps() <= maxSearchDepth);
    assert(qValues.size() == task->getNumberOfActions());

    for(unsigned int index = 0; index < qValues.size(); ++index) {
        if(actionsToExpand[index] == index) {
            applyAction(_rootState, index, qValues[index]);
        }
    }
    return true;
}

void DepthFirstSearch::applyAction(State const& state, int const& actionIndex, double& reward) {
    State nxt(task->getStateSize(), state.remainingSteps()-1, task->getNumberOfStateFluentHashKeys());
    task->calcStateTransitionInDeterminization(state, actionIndex, nxt, reward);

    // Check if the next state is already cached
    if(task->stateValueCache.find(nxt) != task->stateValueCache.end()) {
        reward += task->stateValueCache[nxt];
        return;
    }

    // Check if we have reached a leaf
    if(nxt.remainingSteps() == 1) {
        task->calcOptimalFinalReward(nxt, rewardHelperVar);
        reward += rewardHelperVar;
        return;
    }

    //  Expand the state
    double futureResult = -numeric_limits<double>::max();
    expandState(nxt, futureResult);
    reward += futureResult;
}


void DepthFirstSearch::expandState(State const& state, double& result) {
    assert(task->stateValueCache.find(state) == task->stateValueCache.end());
    assert(MathUtils::doubleIsMinusInfinity(result));

    // Get applicable actions
    vector<int> actionsToExpand = task->getApplicableActions(state);

    // Apply applicable actions and determine best one
    for(unsigned int index = 0; index < actionsToExpand.size(); ++index) {
        if(actionsToExpand[index] == index) {
            double tmp = 0.0;
            applyAction(state, index, tmp);

            if(MathUtils::doubleIsGreater(tmp, result)) {
                result = tmp;
            }
        }
    }

    // Cache state value if caching is enabled
    if(cachingEnabled) {
        task->stateValueCache[state] = result;
    }
}
