#include "iterative_deepening_search.h"

#include "prost_planner.h"
#include "depth_first_search.h"
#include "actions.h"

#include "utils/math_utils.h"
#include "utils/system_utils.h"

#include <iostream>
#include <algorithm>

using namespace std;

/******************************************************************
                                IDS
******************************************************************/

map<State, vector<double>, State::CompareIgnoringRemainingSteps> IterativeDeepeningSearch::rewardCache;

IterativeDeepeningSearch::IterativeDeepeningSearch(ProstPlanner* _planner, vector<double>& _result) :
    SearchEngine("IDS", _planner, _planner->getDeterministicTask(), _result),
    isLearning(false),
    timer(),
    time(0.0),
    terminationTimeout(0.005), 
    strictTerminationTimeout(0.1), 
    terminateWithReasonableAction(true),
    accumulatedSearchDepth(0),
    cacheHits(0),
    numberOfRuns(0) {

    setMinSearchDepth(1);

    elapsedTime.resize(maxSearchDepth+1);
    dfsResult = vector<double>(task->getNumberOfActions()+1,0);

    dfs = new DepthFirstSearch(planner, dfsResult);
    dfs->setMaxSearchDepth(maxSearchDepth);
}

bool IterativeDeepeningSearch::setValueFromString(string& param, string& value) {
    if(param == "-t") {
        setTerminationTimeout(atof(value.c_str()));
        return true;
    } else if(param == "-st") {
        setStrictTerminationTimeout(atof(value.c_str()));
        return true;
    } else if(param == "-tra") {
        setTerminateWithReasonableAction(atoi(value.c_str()));
        return true;
    } else if(param == "-minsd") {
        setMinSearchDepth(atoi(value.c_str()));
        return true;
    }

    return SearchEngine::setValueFromString(param, value);
}

void IterativeDeepeningSearch::setResultType(SearchEngine::ResultType const _resultType) {
    dfs->setResultType(_resultType);
    SearchEngine::setResultType(_resultType);
}

void IterativeDeepeningSearch::setMaxSearchDepth(int _maxSearchDepth) {
    dfs->setMaxSearchDepth(_maxSearchDepth);
    SearchEngine::setMaxSearchDepth(_maxSearchDepth);
    elapsedTime.resize(_maxSearchDepth+1);
}

void IterativeDeepeningSearch::_run() {
    assert(resultType == SearchEngine::ESTIMATE || resultType == SearchEngine::PRUNED_ESTIMATE);
    assert(result.size()+1 == dfsResult.size());

    timer.reset();
    currentState.setTo(rootState);

    map<State, std::vector<double> >::iterator it = IterativeDeepeningSearch::rewardCache.find(currentState);
    if(it != IterativeDeepeningSearch::rewardCache.end()) {
        for(unsigned int i = 0; i < result.size(); ++i) {
            result[i] = it->second[i];
        }
        ++cacheHits;
    } else {
        currentState.remainingSteps() = (task->noopIsOptimalFinalAction() ? 1 : 0);
        do {
            ++currentState.remainingSteps();
            dfs->run(currentState);
        }  while(moreIterations());

        if(cachingEnabled) {
            IterativeDeepeningSearch::rewardCache[currentState] = vector<double>(result.size(),0);
            it = IterativeDeepeningSearch::rewardCache.find(currentState);
            for(unsigned int i = 0; i < result.size(); ++i) {
                if(!MathUtils::doubleIsMinusInfinity(dfsResult[i])) {
                    result[i] = (dfsResult[i] / ((double)currentState.remainingSteps()));
                } else {
                    result[i] = dfsResult[i];
                }
                it->second[i] = result[i];
            }
        } else {
            for(unsigned int i = 0; i < result.size(); ++i) {
                if(!MathUtils::doubleIsMinusInfinity(dfsResult[i])) {
                    result[i] = (dfsResult[i] / ((double)currentState.remainingSteps()));
                } else {
                    result[i] = dfsResult[i];
                }
            }
        }
        accumulatedSearchDepth += currentState.remainingSteps();
        ++numberOfRuns;
    }
}

bool IterativeDeepeningSearch::moreIterations() {
    time = timer();
    if(isLearning) {
        elapsedTime[currentState.remainingSteps()].push_back(time);

        if(MathUtils::doubleIsGreater(time,strictTerminationTimeout)) {
            elapsedTime.resize(currentState.remainingSteps());
            maxSearchDepth = currentState.remainingSteps()-1;
            return false;
        }
    } else {
        if(terminateWithReasonableAction) {
            for(unsigned int i = 1; i < dfsResult.size(); ++i) {
                if(MathUtils::doubleIsGreater(dfsResult[i],dfsResult[0])) {
                    return false;
                }

            }
        }
        if(MathUtils::doubleIsGreater(time,terminationTimeout)) {
            return false;
        }
    }
    return (currentState.remainingSteps() < rootState.remainingSteps());
}

void IterativeDeepeningSearch::resetStats() {
    accumulatedSearchDepth = 0;
    cacheHits = 0;
    numberOfRuns = 0;
}

void IterativeDeepeningSearch::printStats(std::string indent) {
    SearchEngine::printStats(indent);
    if(numberOfRuns > 0) {
        outStream << indent << "Average search depth: " << ((double)accumulatedSearchDepth/(double)numberOfRuns) << " (in " << numberOfRuns << " runs)" << endl;
    }
    outStream << indent << "Maximal search depth: " << maxSearchDepth << endl;
    outStream << indent << "Cache hits: " << cacheHits << endl;
}

void IterativeDeepeningSearch::learn(std::vector<State> const& trainingSet) {
    cout << name << ": learning..." << endl;
    dfs->learn(trainingSet);

    isLearning = true;
    bool cachingEnabledBeforeLearning = cachingEnabled;
    cachingEnabled = false;

    //perform ids for all states in traningSet and record the time it took
    for(unsigned int i = 0; i < trainingSet.size(); ++i) {
        State copy(trainingSet[i]);
        copy.remainingSteps() = maxSearchDepth;
        run(copy);
        if(maxSearchDepth < minSearchDepth) {
            cout << name << ": Setting max search depth to 0!" << endl;
            maxSearchDepth = 0;
            isLearning = false;
            cachingEnabled = cachingEnabledBeforeLearning;
            resetStats();
            return;
        }
    }

    isLearning = false;
    cachingEnabled = cachingEnabledBeforeLearning;
    assert(IterativeDeepeningSearch::rewardCache.empty());

    //determine max search depth based on average time needed in different search depths
    maxSearchDepth = 0;
    unsigned int index = (task->noopIsOptimalFinalAction() ? 2 : 1);

    for(;index < elapsedTime.size(); ++index) {
        if(elapsedTime[index].size() > (trainingSet.size()/2)) {
            double timeSum = 0.0;
            for(unsigned int j = 0; j < elapsedTime[index].size(); ++j) {
                timeSum += elapsedTime[index][j];
            }
            cout << name << ": Search Depth " << index << ": " << timeSum << " / " << elapsedTime[index].size()  
                 << " = " << (timeSum / ((double)elapsedTime[index].size())) << endl;
            if(MathUtils::doubleIsSmaller((timeSum / ((double)elapsedTime[index].size())),terminationTimeout)) {
                maxSearchDepth = index;
            } else {
                break;
            }
        } else {
            break;
        }
    }

    dfs->setMaxSearchDepth(maxSearchDepth);
    cout << name << ": Setting max search depth to " << maxSearchDepth << "!" << endl;
    resetStats();
    cout << name << ": ...finished" << endl;
}
