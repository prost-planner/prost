#include "iterative_deepening_search.h"

#include "depth_first_search.h"
#include "minimal_lookahead_search.h"
#include "prost_planner.h"

#include "utils/logger.h"
#include "utils/math_utils.h"
#include "utils/system_utils.h"

#include <algorithm>
#include <iostream>
#include <numeric>

using namespace std;

/******************************************************************
                     Search Engine Creation
******************************************************************/

IDS::HashMap IDS::rewardCache;

IDS::IDS() :
    DeterministicSearchEngine("IDS"),
    mlh(nullptr),
    isLearning(false),
    stopwatch(),
    maxSearchDepthForThisStep(0),
    ramLimitReached(false),
    strictTerminationTimeout(0.1),
    terminateWithReasonableAction(true),
    accumulatedSearchDepth(0),
    cacheHits(0),
    numberOfRuns(0) {
    setTimeout(0.005);

    if (rewardCache.bucket_count() < 520241) {
        rewardCache.reserve(520241);
    }

    elapsedTime.resize(maxSearchDepth + 1);

    dfs = new DepthFirstSearch();
    dfs->setMaxSearchDepth(maxSearchDepth);
}

bool IDS::setValueFromString(string& param, string& value) {
    if (param == "-st") {
        setStrictTerminationTimeout(atof(value.c_str()));
        return true;
    } else if (param == "-tra") {
        setTerminateWithReasonableAction(atoi(value.c_str()));
        return true;
    }

    return SearchEngine::setValueFromString(param, value);
}

/******************************************************************
                            Parameter
******************************************************************/

void IDS::setMaxSearchDepth(int newValue) {
    dfs->setMaxSearchDepth(newValue);
    SearchEngine::setMaxSearchDepth(newValue);
    elapsedTime.resize(newValue + 1);
}

void IDS::setCachingEnabled(bool newValue) {
    SearchEngine::setCachingEnabled(newValue);
    dfs->setCachingEnabled(newValue);
    if (mlh) {
        mlh->setCachingEnabled(newValue);
    }
}

/******************************************************************
                 Search Engine Administration
******************************************************************/

void IDS::disableCaching() {
    dfs->disableCaching();
    SearchEngine::disableCaching();
    ramLimitReached = true;
    if (mlh) {
        mlh->disableCaching();
    }
}

void IDS::learn() {
    dfs->learn();
    Logger::logLine(name + ": learning...", Verbosity::NORMAL);

    isLearning = true;
    bool cachingEnabledBeforeLearning = cachingIsEnabled();
    cachingEnabled = false;

    // Perform IDS for all states in trainingSet and record the time it takes
    for (size_t i = 0; i < trainingSet.size(); ++i) {
        State copy(trainingSet[i]);
        vector<double> res(numberOfActions);
        vector<int> actionsToExpand = getApplicableActions(copy);
        estimateQValues(copy, actionsToExpand, res);
    }

    isLearning = false;
    cachingEnabled = cachingEnabledBeforeLearning;
    assert(rewardCache.empty());

    if (maxSearchDepth > 1) {
        // Determine the maximal search depth based on the average time
        // the search needed on the training set
        maxSearchDepth = 0;

        for (size_t index = 2; index < elapsedTime.size(); ++index) {
            vector<double> const& times = elapsedTime[index];
            if (times.size() <= (trainingSet.size() / 2)) {
                break;
            }
            double timeSum = std::accumulate(times.begin(), times.end(), 0.0);
            double avgTime = timeSum / static_cast<double>(times.size());

            Logger::logLine(
                name + ": Search Depth " + to_string(index) + ": " +
                to_string(timeSum) + " / " + to_string(times.size()) +
                " = " + to_string(avgTime), Verbosity::NORMAL);
            if (MathUtils::doubleIsGreaterOrEqual(avgTime, timeout)) {
                break;
            }
            maxSearchDepth = index;
        }
    }

    if (maxSearchDepth <= 1) {
        Logger::logLine(name + ": Learned max search depth is too low: " +
                        to_string(maxSearchDepth) + ". Replacing IDS with " +
                        "minimal lookahead search.", Verbosity::SILENT);
        createMinimalLookaheadSearch();
    } else {
        setMaxSearchDepth(maxSearchDepth);
        Logger::logLine(name + ": Setting max search depth to: " +
                        to_string(maxSearchDepth), Verbosity::SILENT);
    }
    resetStats();
    Logger::logLine(name + ": ...finished", Verbosity::NORMAL);
}

void IDS::createMinimalLookaheadSearch() {
    assert(!mlh);
    mlh = new MinimalLookaheadSearch();
    mlh->setCachingEnabled(cachingEnabled);
    rewardCache.clear();
}

/******************************************************************
                       Main Search Functions
******************************************************************/

void IDS::estimateQValue(State const& state, int actionIndex, double& qValue) {
    if (mlh) {
        // It would also be possible to check the rewardCache first and use the
        // result from there if there is one, but then we mix apples and
        // oranges in the heuristic computation which is presumably worse than
        // just sticking to the result of mlh always.
        return mlh->estimateQValue(state, actionIndex, qValue);
    }

    HashMap::iterator it = rewardCache.find(state);
    if (it != rewardCache.end() &&
        !MathUtils::doubleIsMinusInfinity(it->second[actionIndex])) {
        ++cacheHits;
        qValue =
            it->second[actionIndex] * static_cast<double>(state.stepsToGo());
    } else {
        stopwatch.reset();

        maxSearchDepthForThisStep = std::min(maxSearchDepth, state.stepsToGo());

        State currentState(state);
        currentState.stepsToGo() = 1;
        do {
            ++currentState.stepsToGo();
            dfs->estimateQValue(currentState, actionIndex, qValue);
        } while (moreIterations(currentState.stepsToGo()));

        qValue /= static_cast<double>(currentState.stepsToGo());

        // TODO: Currently, we cache every result, but we should only do so if
        // the result was achieved with a reasonable action, with a timeout or
        // on a state with sufficient depth
        if (cachingEnabled) {
            if (it == rewardCache.end()) {
                rewardCache[currentState] =
                    vector<double>(SearchEngine::numberOfActions,
                                   -std::numeric_limits<double>::max());
            }
            rewardCache[currentState][actionIndex] = qValue;
        }
        qValue *= static_cast<double>(state.stepsToGo());

        accumulatedSearchDepth += currentState.stepsToGo();
        ++numberOfRuns;
    }
}

void IDS::estimateQValues(State const& state,
                          vector<int> const& actionsToExpand,
                          vector<double>& qValues) {
    if (mlh) {
        // It would also be possible to check the rewardCache first and use the
        // result from there if there is one, but then we mix apples and
        // oranges in the heuristic computation which is presumably worse than
        // just sticking to the result of mlh always.
        return mlh->estimateQValues(state, actionsToExpand, qValues);
    }

    HashMap::iterator it = rewardCache.find(state);
    if (it != rewardCache.end()) {
        ++cacheHits;
        assert(qValues.size() == it->second.size());
        for (size_t index = 0; index < qValues.size(); ++index) {
            if (actionsToExpand[index] == index) {
                qValues[index] =
                    it->second[index] * static_cast<double>(state.stepsToGo());
            } else {
                qValues[index] = -std::numeric_limits<double>::max();
            }
        }
    } else {
        stopwatch.reset();

        maxSearchDepthForThisStep = std::min(maxSearchDepth, state.stepsToGo());

        State currentState(state);
        currentState.stepsToGo() = 1;
        do {
            ++currentState.stepsToGo();
            dfs->estimateQValues(currentState, actionsToExpand, qValues);
        } while (
            moreIterations(currentState.stepsToGo(), actionsToExpand, qValues));

        double multiplier = static_cast<double>(state.stepsToGo()) /
                            static_cast<double>(currentState.stepsToGo());
        // TODO: Currently, we cache every result, but we should only do so if
        // the result was achieved with a reasonable action, with a timeout or
        // on a state with sufficient depth
        if (cachingEnabled) {
            rewardCache[currentState] = qValues;
            for (size_t index = 0; index < qValues.size(); ++index) {
                if (actionsToExpand[index] == index) {
                    qValues[index] *= multiplier;
                    rewardCache[currentState][index] /=
                        static_cast<double>(currentState.stepsToGo());
                }
            }
        } else {
            for (size_t index = 0; index < qValues.size(); ++index) {
                if (actionsToExpand[index] == index) {
                    qValues[index] *= multiplier;
                }
            }
        }

        accumulatedSearchDepth += currentState.stepsToGo();
        ++numberOfRuns;
    }
}

bool IDS::moreIterations(int const& stepsToGo) {
    double time = stopwatch();

    // 1. If caching was disabled, we check if the strict timeout is violated to
    // readjust the maximal search depth
    if (ramLimitReached &&
        MathUtils::doubleIsGreater(time, strictTerminationTimeout)) {
        if (maxSearchDepth == 1) {
            Logger::logLine(
                name + ": Timeout violated (" + to_string(time) + "s). " +
                "on minimal search depth. Cannot decrease max search depth " +
                "anymore. Replacing IDS with minimal lookahead search.",
                Verbosity::SILENT);
            createMinimalLookaheadSearch();
        } else {
            Logger::logLine(name + ": Timeout violated (" + to_string(time) +
                            "s). Setting max search depth to: " +
                            to_string(stepsToGo - 1), Verbosity::SILENT);
            setMaxSearchDepth(stepsToGo - 1);
        }
        return false;
    }

    // 2. Check if we have reached the max search depth for this step
    return stepsToGo < maxSearchDepthForThisStep;
}

bool IDS::moreIterations(int const& stepsToGo,
                         vector<int> const& actionsToExpand,
                         vector<double>& qValues) {
    double time = stopwatch();

    // 0. If we are learning, we apply different termination criteria
    if (isLearning) {
        assert(elapsedTime.size() > stepsToGo);
        elapsedTime[stepsToGo].push_back(time);

        if (MathUtils::doubleIsGreater(time, strictTerminationTimeout)) {
            // We require at least 3 entries in elapsedTime, since DFS always
            // begins with 2 stepsToGo.
            elapsedTime.resize(std::max(3, stepsToGo));
            maxSearchDepth = std::max(stepsToGo - 1, 1);
            return false;
        }
        return stepsToGo < maxSearchDepthForThisStep;
    }

    // 1. If caching was disabled, we check if the strict timeout is violated to
    // readjust the maximal search depth
    if (ramLimitReached &&
        MathUtils::doubleIsGreater(time, strictTerminationTimeout)) {
        if (maxSearchDepth == 1) {
            Logger::logLine(
                name + ": Timeout violated (" + to_string(time) + "s). " +
                "on minimal search depth. Cannot decrease max search depth " +
                "anymore. Replacing IDS with minimal lookahead search.",
                Verbosity::SILENT);
            createMinimalLookaheadSearch();
        } else {
            Logger::logLine(name + ": Timeout violated (" + to_string(time) +
                            "s). Setting max search depth to: " +
                            to_string(stepsToGo - 1), Verbosity::SILENT);
            setMaxSearchDepth(stepsToGo - 1);
        }
        return false;
    }

    // 2. Check if the result is already significant (if noop is applicable, we
    // check if there is an action that yields a higher reward than noop)
    if (terminateWithReasonableAction &&
        actionStates[0].scheduledActionFluents.empty() &&
        (actionsToExpand[0] == 0)) {
        for (size_t index = 1; index < qValues.size(); ++index) {
            if ((actionsToExpand[index] == index) &&
                MathUtils::doubleIsGreater(qValues[index], qValues[0])) {
                return false;
            }
        }
    }

    // 3. Check if we have reached the max search depth for this step
    return stepsToGo < maxSearchDepthForThisStep;
}

/******************************************************************
                   Statistics and Prints
******************************************************************/

void IDS::resetStats() {
    accumulatedSearchDepth = 0;
    cacheHits = 0;
    numberOfRuns = 0;
    if (mlh) {
        mlh->resetStats();
    }
}

void IDS::printStats(
        bool const& printRoundStats, string indent) const {
    SearchEngine::printStats(printRoundStats, indent);
    if (mlh) {
        mlh->printStats(printRoundStats, indent);
        Logger::logLine(indent + "Cache hits of IDS: " + to_string(cacheHits),
                        Verbosity::NORMAL);
    } else {
        if (numberOfRuns > 0) {
            double avg = static_cast<double>(accumulatedSearchDepth) /
                         numberOfRuns;
            Logger::logLine(indent + "Average search depth: " +
                            to_string(avg) + " (in " + to_string(numberOfRuns) +
                            " runs)", Verbosity::NORMAL);
        }
        Logger::logLine(indent + "Maximal search depth: " +
                        to_string(maxSearchDepth), Verbosity::NORMAL);
        Logger::logLine(indent + "Cache hits: " + to_string(cacheHits),
                        Verbosity::NORMAL);
    }
}
