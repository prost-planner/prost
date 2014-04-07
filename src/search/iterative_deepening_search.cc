#include "iterative_deepening_search.h"

#include "prost_planner.h"
#include "planning_task.h"
#include "depth_first_search.h"

#include "utils/math_utils.h"
#include "utils/system_utils.h"

#include <iostream>
#include <algorithm>

using namespace std;

/******************************************************************
                     Search Engine Creation
******************************************************************/

map<State, vector<double>, State::CompareIgnoringRemainingSteps> IterativeDeepeningSearch::rewardCache;

IterativeDeepeningSearch::IterativeDeepeningSearch(ProstPlanner* _planner, PlanningTask* _task) :
    SearchEngine("IDS", _planner, _task, true),
    currentState(_task->getStateSize(), -1, _task->getNumberOfStateFluentHashKeys()),
    isLearning(false),
    timer(),
    time(0.0),
    maxSearchDepthForThisStep(0),
    terminationTimeout(0.005), 
    strictTerminationTimeout(0.1), 
    terminateWithReasonableAction(true),
    accumulatedSearchDepth(0),
    cacheHits(0),
    numberOfRuns(0) {

    setMinSearchDepth(2);

    elapsedTime.resize(maxSearchDepth+1);

    dfs = new DepthFirstSearch(planner, task);
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

/******************************************************************
                            Parameter
******************************************************************/

void IterativeDeepeningSearch::setMaxSearchDepth(int _maxSearchDepth) {
    dfs->setMaxSearchDepth(_maxSearchDepth);
    SearchEngine::setMaxSearchDepth(_maxSearchDepth);
    elapsedTime.resize(_maxSearchDepth+1);
}

void IterativeDeepeningSearch::setCachingEnabled(bool _cachingEnabled) {
    SearchEngine::setCachingEnabled(_cachingEnabled);
    dfs->setCachingEnabled(_cachingEnabled);
}

/******************************************************************
                 Search Engine Administration
******************************************************************/

void IterativeDeepeningSearch::disableCaching() {
    dfs->disableCaching();
    SearchEngine::disableCaching();
}

void IterativeDeepeningSearch::learn(std::vector<State> const& trainingSet) {
    dfs->learn(trainingSet);
    cout << name << ": learning..." << endl;

    isLearning = true;
    bool cachingEnabledBeforeLearning = cachingIsEnabled();
    cachingEnabled = false;

    // Perform IDS for all states in trainingSet and record the time it takes
    for(unsigned int i = 0; i < trainingSet.size(); ++i) {
        State copy(trainingSet[i]);
        vector<double> res(task->getNumberOfActions());

        vector<int> actionsToExpand = getApplicableActions(copy);

        estimateQValues(copy, actionsToExpand, res);
        if(maxSearchDepth < minSearchDepth) {
            cout << name << ": Setting max search depth to 0!" << endl;
            setMaxSearchDepth(0);
            isLearning = false;
            cachingEnabled = cachingEnabledBeforeLearning;
            resetStats();
            return;
        }
    }

    isLearning = false;
    cachingEnabled = cachingEnabledBeforeLearning;
    assert(IterativeDeepeningSearch::rewardCache.empty());

    // Determine the maximal search depth based on the average time the search
    // needed on the training set
    maxSearchDepth = 0;
    unsigned int index = 2;

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

    setMaxSearchDepth(maxSearchDepth);
    cout << name << ": Setting max search depth to " << maxSearchDepth << "!" << endl;
    resetStats();
    cout << name << ": ...finished" << endl;
}

/******************************************************************
                       Main Search Functions
******************************************************************/

bool IterativeDeepeningSearch::estimateQValues(State const& _rootState, vector<int> const& actionsToExpand, vector<double>& qValues) {
    timer.reset();

    if(_rootState.remainingSteps() > maxSearchDepth) {
        maxSearchDepthForThisStep = maxSearchDepth;
    } else {
        maxSearchDepthForThisStep = _rootState.remainingSteps();
    }

    map<State, std::vector<double> >::iterator it = IterativeDeepeningSearch::rewardCache.find(_rootState);
    if(it != IterativeDeepeningSearch::rewardCache.end()) {
        ++cacheHits;
        assert(qValues.size() == it->second.size());
        for(unsigned int i = 0; i < qValues.size(); ++i) {
            qValues[i] = it->second[i];
        }
    } else {
        currentState.setTo(_rootState);
        currentState.remainingSteps() = 1;
        do {
            ++currentState.remainingSteps();
            dfs->estimateQValues(currentState, actionsToExpand, qValues);
        }  while(moreIterations(actionsToExpand, qValues));

        for(unsigned int actionIndex = 0; actionIndex < qValues.size(); ++actionIndex) {
            if(actionsToExpand[actionIndex] == actionIndex) {
                qValues[actionIndex] /= ((double)currentState.remainingSteps());
            }
        }

        // TODO: Currently, we cache every result, but we should only do so if
        // the result was achieved with a reasonable action, with a timeout or
        // on a state with sufficient depth
        if(cachingEnabled) {
            IterativeDeepeningSearch::rewardCache[currentState] = qValues;
        }

        accumulatedSearchDepth += currentState.remainingSteps();
        ++numberOfRuns;
    }

    return true;
}

bool IterativeDeepeningSearch::moreIterations(vector<int> const& actionsToExpand, vector<double>& qValues) {
    time = timer();

    // If we are learning, we apply different termination criteria
    if(isLearning) {
        elapsedTime[currentState.remainingSteps()].push_back(time);

        if(MathUtils::doubleIsGreater(time,strictTerminationTimeout)) {
            elapsedTime.resize(currentState.remainingSteps());
            maxSearchDepth = currentState.remainingSteps()-1;
            return false;
        }
        return (currentState.remainingSteps() < maxSearchDepthForThisStep);
    }

    // 1. Check if we have a significant result
    if(terminateWithReasonableAction) {
        if(actionsToExpand[0] == 0) {
            // Noop is applicable -> we check if another action is better than
            // noop
            for(unsigned int actionIndex = 1; actionIndex < qValues.size(); ++actionIndex) {
                if((actionsToExpand[actionIndex] == actionIndex) && 
                   MathUtils::doubleIsGreater(qValues[actionIndex], qValues[0])) {
                    return false;
                }
            }
        } else {
            // Noop is not applicable -> we determine the first applicable
            // action
            unsigned int firstApplicableActionIndex = 1;
            while(actionsToExpand[firstApplicableActionIndex] != firstApplicableActionIndex) {
    		++firstApplicableActionIndex;
            }

            // There must be at least one applicable action
            assert(firstApplicableActionIndex < qValues.size());

            // Check if any two applicable actions yield different results
            for(unsigned int actionIndex = firstApplicableActionIndex; actionIndex < qValues.size(); ++actionIndex) {
                if((actionsToExpand[actionIndex] == actionIndex) && 
                   !MathUtils::doubleIsEqual(qValues[actionIndex], qValues[firstApplicableActionIndex])) {
                    return false;
                }
            }
        }
    }

    // 2. Check if we have reached the max search depth for this step
    return currentState.remainingSteps() < maxSearchDepthForThisStep;
}

/******************************************************************
                   Statistics and Printers
******************************************************************/

void IterativeDeepeningSearch::resetStats() {
    accumulatedSearchDepth = 0;
    cacheHits = 0;
    numberOfRuns = 0;
}

void IterativeDeepeningSearch::printStats(ostream& out, bool const& printRoundStats, string indent) {
    SearchEngine::printStats(out, printRoundStats, indent);
    if(numberOfRuns > 0) {
        out << indent << "Average search depth: " << ((double)accumulatedSearchDepth/(double)numberOfRuns) << " (in " << numberOfRuns << " runs)" << endl;
    }
    out << indent << "Maximal search depth: " << maxSearchDepth << endl;
    out << indent << "Cache hits: " << cacheHits << endl;
}
