#include "iterative_deepening_search.h"

#include "prost_planner.h"
#include "depth_first_search.h"

#include "utils/math_utils.h"
#include "utils/system_utils.h"

#include <iostream>
#include <algorithm>

using namespace std;

/******************************************************************
                     Search Engine Creation
******************************************************************/

IDS::HashMap IDS::rewardCache(520241);

IDS::IDS() :
    DeterministicSearchEngine("IDS"),
    currentState(State()),
    isLearning(false),
    timer(),
    time(0.0),
    maxSearchDepthForThisStep(0),
    ramLimitReached(false),
    terminationTimeout(0.005),
    strictTerminationTimeout(0.1),
    terminateWithReasonableAction(true),
    accumulatedSearchDepth(0),
    cacheHits(0),
    numberOfRuns(0) {
    setMinSearchDepth(2);

    elapsedTime.resize(maxSearchDepth + 1);

    dfs = new DepthFirstSearch();
    dfs->setMaxSearchDepth(maxSearchDepth);
}

bool IDS::setValueFromString(string& param, string& value) {
    if (param == "-t") {
        setTerminationTimeout(atof(value.c_str()));
        return true;
    } else if (param == "-st") {
        setStrictTerminationTimeout(atof(value.c_str()));
        return true;
    } else if (param == "-tra") {
        setTerminateWithReasonableAction(atoi(value.c_str()));
        return true;
    } else if (param == "-minsd") {
        setMinSearchDepth(atoi(value.c_str()));
        return true;
    }

    return SearchEngine::setValueFromString(param, value);
}

/******************************************************************
                            Parameter
******************************************************************/

void IDS::setMaxSearchDepth(int _maxSearchDepth) {
    dfs->setMaxSearchDepth(_maxSearchDepth);
    SearchEngine::setMaxSearchDepth(_maxSearchDepth);
    elapsedTime.resize(_maxSearchDepth + 1);
}

void IDS::setCachingEnabled(bool _cachingEnabled) {
    SearchEngine::setCachingEnabled(_cachingEnabled);
    dfs->setCachingEnabled(_cachingEnabled);
}

/******************************************************************
                 Search Engine Administration
******************************************************************/

void IDS::disableCaching() {
    dfs->disableCaching();
    SearchEngine::disableCaching();
    ramLimitReached = true;
}

void IDS::learn() {
    dfs->learn();
    cout << name << ": learning..." << endl;

    isLearning = true;
    bool cachingEnabledBeforeLearning = cachingIsEnabled();
    cachingEnabled = false;

    // Perform IDS for all states in trainingSet and record the time it takes
    for (unsigned int i = 0; i < SearchEngine::trainingSet.size(); ++i) {
        State copy(SearchEngine::trainingSet[i]);
        vector<double> res(SearchEngine::numberOfActions);

        vector<int> actionsToExpand = getApplicableActions(copy);

        estimateQValues(copy, actionsToExpand, res);
        if (maxSearchDepth < minSearchDepth) {
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
    assert(rewardCache.empty());

    // Determine the maximal search depth based on the average time the search
    // needed on the training set
    maxSearchDepth = 0;
    unsigned int index = 2;

    for (; index < elapsedTime.size(); ++index) {
        if (elapsedTime[index].size() >
            (SearchEngine::trainingSet.size() / 2)) {
            double timeSum = 0.0;
            for (unsigned int j = 0; j < elapsedTime[index].size(); ++j) {
                timeSum += elapsedTime[index][j];
            }
            cout << name << ": Search Depth " << index << ": " << timeSum <<
            " / " << elapsedTime[index].size()
                 << " = " <<
            (timeSum / ((double) elapsedTime[index].size())) << endl;
            if (MathUtils::doubleIsSmaller((timeSum / ((double) elapsedTime[index].size())),terminationTimeout)) {
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

bool IDS::estimateQValues(State const& _rootState,
                          vector<int> const& actionsToExpand,
                          vector<double>& qValues) {
    timer.reset();

    if (_rootState.remainingSteps() > maxSearchDepth) {
        maxSearchDepthForThisStep = maxSearchDepth;
    } else {
        maxSearchDepthForThisStep = _rootState.remainingSteps();
    }

    HashMap::iterator it = rewardCache.find(_rootState);

    if (it != rewardCache.end()) {
        ++cacheHits;
        assert(qValues.size() == it->second.size());
        for (unsigned int i = 0; i < qValues.size(); ++i) {
            qValues[i] = it->second[i];
        }
    } else {
        currentState.setTo(_rootState);
        currentState.remainingSteps() = 1;
        do {
            ++currentState.remainingSteps();
            dfs->estimateQValues(currentState, actionsToExpand, qValues);
        }  while (moreIterations(actionsToExpand, qValues));

        for (unsigned int actionIndex = 0; actionIndex < qValues.size();
             ++actionIndex) {
            if (actionsToExpand[actionIndex] == actionIndex) {
                qValues[actionIndex] /= ((double) currentState.remainingSteps());
            }
        }

        // TODO: Currently, we cache every result, but we should only do so if
        // the result was achieved with a reasonable action, with a timeout or
        // on a state with sufficient depth
        if (cachingEnabled) {
            rewardCache[currentState] = qValues;
        }

        accumulatedSearchDepth += currentState.remainingSteps();
        ++numberOfRuns;
    }

    return true;
}

bool IDS::moreIterations(vector<int> const& actionsToExpand,
                         vector<double>& qValues) {
    time = timer();

    // If we are learning, we apply different termination criteria
    if (isLearning) {
        elapsedTime[currentState.remainingSteps()].push_back(time);

        if (MathUtils::doubleIsGreater(time, strictTerminationTimeout)) {
            elapsedTime.resize(currentState.remainingSteps());
            maxSearchDepth = currentState.remainingSteps() - 1;
            return false;
        }
        return currentState.remainingSteps() < maxSearchDepthForThisStep;
    }

    // 1. If caching was disabled, we check if the strict timeout is violated to
    // readjust the maximal search depth
    if(ramLimitReached && MathUtils::doubleIsGreater(time, strictTerminationTimeout)) {
        maxSearchDepth = currentState.remainingSteps()-1;
        if((currentState.remainingSteps() - 1) < minSearchDepth) {
            cout << name << ": Timeout violated (" << time 
                 << "s). Setting max search depth to " 
                 << minSearchDepth << "!" << endl;
            setMaxSearchDepth(minSearchDepth);
        } else {
            cout << name << ": Timeout violated (" << time 
                 << "s). Setting max search depth to " 
                 << (currentState.remainingSteps() - 1) << "!" << endl;
            setMaxSearchDepth(currentState.remainingSteps()-1);
        }
        return false;
    }

    // 2. Check if the result is already significant (if noop is applicable, we
    // check if there is an action that yields a higher reward than noop)
    if(terminateWithReasonableAction && SearchEngine::actionStates[0].scheduledActionFluents.empty() && (actionsToExpand[0] == 0)) {
        for(unsigned int actionIndex = 1; actionIndex < qValues.size(); ++actionIndex) {
            if((actionsToExpand[actionIndex] == actionIndex) && 
               MathUtils::doubleIsGreater(qValues[actionIndex], qValues[0])) {
                return false;
            }
        }
    }

    // 3. Check if we have reached the max search depth for this step
    return currentState.remainingSteps() < maxSearchDepthForThisStep;
}

/******************************************************************
                   Statistics and Printers
******************************************************************/

void IDS::resetStats() {
    accumulatedSearchDepth = 0;
    cacheHits = 0;
    numberOfRuns = 0;
}

void IDS::printStats(ostream& out, bool const& printRoundStats, string indent) const {
    SearchEngine::printStats(out, printRoundStats, indent);
    if (numberOfRuns > 0) {
        out << indent << "Average search depth: " <<
            ((double) accumulatedSearchDepth /
             (double) numberOfRuns) << " (in " << numberOfRuns << " runs)" << endl;
    }
    out << indent << "Maximal search depth: " << maxSearchDepth << endl;
    out << indent << "Cache hits: " << cacheHits << endl;
}
