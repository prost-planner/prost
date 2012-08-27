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

DepthFirstSearch::DepthFirstSearch(ProstPlanner* _planner, vector<double>& _result) :
    SearchEngine("DFS", _planner, _planner->getDeterministicTask(), _result),
    evaluatedStates(0),
    cacheHits(0),
    rewardHelperVar(0.0),
    referenceReward(0.0) {

    bestRewardIndex = task->getNumberOfActions();

    //TODO: Check why adding this makes such a huge difference (e.g. with sysadmin 5)!
    if(task->stateHashingPossible()) {
        cachingEnabled = true;
    } else {
        cachingEnabled = false;
    }
}

int DepthFirstSearch::getNumberOfEvaluatedStates() {
    return evaluatedStates;
}

int DepthFirstSearch::getNumberOfCacheHits() {
    return cacheHits;
}

void DepthFirstSearch::_run() {
    assert(resultType == SearchEngine::ESTIMATE || resultType == SearchEngine::PRUNED_ESTIMATE);
    assert(rootState.remainingSteps() > 0);

    currentState.setTo(rootState);

    evaluatedStates = 0;
    cacheHits = 0;

    map<State, vector<double> >::iterator it = task->rewardCache.find(currentState);
    if(it != task->rewardCache.end()) {
        ++cacheHits;
        assert(result.size() == it->second.size());
        for(unsigned int i = 0; i < result.size(); ++i) {
            result[i] = it->second[i];
        }
    } else {
        vector<double> res(bestRewardIndex+1,-numeric_limits<double>::max());

        if(currentState.remainingSteps() == 1 && task->noopIsOptimalFinalAction()) {
            rewardHelperVar = 0.0;
            task->calcReward(currentState, 0, currentState, rewardHelperVar);
            vector<int> expands(actionsToExpand.size(),-1);
            task->setActionsToExpand(currentState, expands);
            for(unsigned int index = 0; index < expands.size(); ++index) {
                if(expands[index] == index) {
                    res[index] = rewardHelperVar;
                }
            }
            res[bestRewardIndex] = rewardHelperVar;
        } else {
            expandState(currentState, res);
        }

        if(cachingEnabled) {
            task->rewardCache[currentState] = vector<double>(bestRewardIndex+1);
            it = task->rewardCache.find(currentState);
            for(unsigned int i = 0; i < res.size(); ++i) {
                it->second[i] = res[i];
                result[i] = res[i];
            }
        } else {
            for(unsigned int i = 0; i < result.size(); ++i) {
                result[i] = res[i];
            }
        }
    }
}

void DepthFirstSearch::expandState(State& state, vector<double>& res) {
    vector<int> expands(actionsToExpand.size(),-1);
    task->setActionsToExpand(state, expands);

    /*
    if(task->isARewardLock(state, referenceReward)) {
        if(resultType == SearchEngine::ESTIMATE) {
            for(unsigned int index = 0; index < expands.size(); ++index) {
                res[index] = state.remainingSteps() * referenceReward;
            }
        } else {
            for(unsigned int index = 0; index < expands.size(); ++index) {
                if(expands[index] == index) {
                    res[index] = state.remainingSteps() * referenceReward;
                }
            }
            res[bestRewardIndex] = state.remainingSteps() * referenceReward;
            return;
        }
    }
    */

    for(unsigned int index = 0; index < expands.size(); ++index) {
        if(expands[index] == index) {
            applyAction(state, index, res[index]);

            if(MathUtils::doubleIsGreater(res[index], res[bestRewardIndex])) {
                res[bestRewardIndex] = res[index];

                /*
                //TODO: If the currently best reward is equal to the maximal possible reward, we can stop here!
                if(res[bestRewardIndex] == (task->getMaxReward() * state.remainingSteps())) {
                    return;
                }
                */
            }
        } else {
            if(resultType == SearchEngine::ESTIMATE) {
                assert(expands[index] < index);
                res[index] = res[expands[index]];
            } else {
                assert(MathUtils::doubleIsMinusInfinity(res[index]));
            }
        }
    }
}

void DepthFirstSearch::applyAction(State& state, int actionIndex, double& reward) {
    ++evaluatedStates;

    State nxt(task->getStateSize(), state.remainingSteps()-1);
    task->calcStateTransition(state, actionIndex, nxt, reward);

    if(nxt.remainingSteps() == 1 && task->noopIsOptimalFinalAction()) {
        rewardHelperVar = 0.0;
        task->calcReward(nxt, 0, nxt, rewardHelperVar);
        reward += rewardHelperVar;
        return;
    } else if(nxt.remainingSteps() == 0) {
        return;
    }

    if(task->rewardCache.find(nxt) == task->rewardCache.end()) {
        vector<double> res(bestRewardIndex+1,-numeric_limits<double>::max());
        expandState(nxt, res);
        if(cachingEnabled) {
            task->rewardCache[nxt] = res;
        }
        reward += res[bestRewardIndex];
    } else {
        ++cacheHits;
        reward += task->rewardCache[nxt][bestRewardIndex];
    }
}
