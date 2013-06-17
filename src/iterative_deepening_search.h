#ifndef ITERATIVE_DEEPENING_SEARCH_H
#define ITERATIVE_DEEPENING_SEARCH_H

// Implements an iterative deepending search engine. This was used as
// initialization for UCT in IPPC 2011 and is described in the paper
// by Keller and Eyerich (2012).

#include "search_engine.h"

#include "utils/timer.h"

#include <map>

class ProstPlanner;
class DepthFirstSearch;

class IterativeDeepeningSearch : public SearchEngine {
public:
    IterativeDeepeningSearch(ProstPlanner* _planner);

    // Set parameters from command line
    bool setValueFromString(std::string& param, std::string& value);

    // Learn parameter values from a training set
    bool learn(std::vector<State> const& trainingSet);

    // Start the search engine for Q-value estimation
    void estimateQValues(State const& _rootState, std::vector<double>& result, bool const& pruneResult);

    // Parameter setter
    void setMaxSearchDepth(int _maxSearchDepth);

    virtual void setTerminationTimeout(double _terminationTimeout) {
        terminationTimeout = _terminationTimeout;
    }

    virtual void setStrictTerminationTimeout(double _strictTerminationTimeout) {
        strictTerminationTimeout = _strictTerminationTimeout;
    }

    virtual void setTerminateWithReasonableAction(bool _terminateWithReasonableAction) {
        terminateWithReasonableAction = _terminateWithReasonableAction;
    }

    virtual void setMinSearchDepth(int _minSearchDepth) {
        minSearchDepth = _minSearchDepth;
    }

    virtual void setCachingEnabled(bool _cachingEnabled) {
        SearchEngine::setCachingEnabled(_cachingEnabled);
        dfs->setCachingEnabled(_cachingEnabled);
    }

    // Reset statistic variables
    void resetStats();

    // Printer
    void printStats(std::ostream& out, bool const& printRoundStats, std::string indent = "");

protected:
    // Decides whether more iterations are possible and reasonable
    bool moreIterations(std::vector<double>& result);

    // The state that is given iteratively to the DFS engine
    State currentState;

    // Learning related variables
    bool isLearning;
    std::vector<std::vector<double> > elapsedTime;

    // Timer related variables
    Timer timer;
    double time;

    // The depth first search engine
    SearchEngine* dfs;

    // The number of remaining steps for this step
    int maxSearchDepthForThisStep;

    // Parameter
    double terminationTimeout;
    double strictTerminationTimeout;
    bool terminateWithReasonableAction;
    int minSearchDepth;

    // Statistics
    int accumulatedSearchDepth;
    int cacheHits;
    int numberOfRuns;

    // Caching
    static std::map<State, std::vector<double>, State::CompareIgnoringRemainingSteps> rewardCache;
};

#endif
