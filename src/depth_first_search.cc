#include "depth_first_search.h"

#include "prost_planner.h"
#include "actions.h"
#include "conditional_probability_functions.h"

#include "utils/math_utils.h"

#include <iostream>
#include <set>

using namespace std;

/******************************************************************
            Constructor, Statistics and Parameters
******************************************************************/

DepthFirstSearch::DepthFirstSearch(ProstPlanner* _planner) :
    SearchEngine("DFS", _planner, _planner->getDeterministicTask()),
    rewardHelperVar(0.0) {

    //TODO: Check why adding this makes such a huge difference (e.g.
    //with sysadmin 5)!
    if(task->stateHashingPossible()) {
        cachingEnabled = true;
    } else {
        cachingEnabled = false;
    }
}

void DepthFirstSearch::estimateQValues(State const& _rootState, vector<double>& result, bool const& pruneResult) {
    assert(_rootState.remainingSteps() > 0);
    assert(_rootState.remainingSteps() <= maxSearchDepth);
    assert(result.size() == task->getNumberOfActions());

    if(pruneResult) {
        vector<int> actionsToExpand(task->getNumberOfActions(),-1);
        task->setActionsToExpand(_rootState, actionsToExpand);

        for(unsigned int index = 0; index < result.size(); ++index) {
            if(actionsToExpand[index] == index) {
                applyAction(_rootState, index, result[index]);
            } else {
                result[index] = -numeric_limits<double>::max();
            }
        }
    } else {
        for(unsigned int index = 0; index < result.size(); ++index) {
            applyAction(_rootState, index, result[index]);
        }
    }
}

void DepthFirstSearch::applyAction(State const& state, int const& actionIndex, double& reward) {
    State nxt(task->getStateSize(), state.remainingSteps()-1);
    task->calcStateTransition(state, actionIndex, nxt, reward);

    //1. check if the next state is already cached
    if(task->stateValueCache.find(nxt) != task->stateValueCache.end()) {
        reward += task->stateValueCache[nxt];
        return;
    }

    //2. Check if we have reached a leaf
    if(nxt.remainingSteps() == 0) {
        return;
    } else if(nxt.remainingSteps() == 1 && task->noopIsOptimalFinalAction()) {
        rewardHelperVar = 0.0;
        State final(task->getStateSize(), 0);
        task->calcReward(nxt, 0, final, rewardHelperVar);
        reward += rewardHelperVar;
        return;
    }

    //3. Expand the state
    double futureResult = -numeric_limits<double>::max();
    expandState(nxt, futureResult);
    reward += futureResult;
}


void DepthFirstSearch::expandState(State const& state, double& result) {
    assert(task->stateValueCache.find(state) == task->stateValueCache.end());
    assert(MathUtils::doubleIsMinusInfinity(result));

    //Calculate reasonable actions
    vector<int> actionsToExpand(task->getNumberOfActions(), -1);
    task->setActionsToExpand(state, actionsToExpand);

    //Apply reasonable actions and determine best one
    for(unsigned int index = 0; index < actionsToExpand.size(); ++index) {
        if(actionsToExpand[index] == index) {
            double tmp = 0.0;
            applyAction(state, index, tmp);

            if(MathUtils::doubleIsGreater(tmp, result)) {
                result = tmp;
            }
        }
    }

    //Cache state value if caching is enabled
    if(cachingEnabled) {
        task->stateValueCache[state] = result;
    }
}
