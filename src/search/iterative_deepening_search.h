#ifndef ITERATIVE_DEEPENING_SEARCH_H
#define ITERATIVE_DEEPENING_SEARCH_H

// Implements an iterative deepending search engine. This was used as
// initialization for UCT in IPPC 2011 (and IPPC 2014) and is described in the
// paper by Keller and Eyerich (ICAPS 2012).

#include "search_engine.h"
#include "states.h"

#include "utils/stopwatch.h"

#include <unordered_map>

class DepthFirstSearch;

class IDS : public DeterministicSearchEngine {
public:
    IDS();

    // Set parameters from command line
    bool setValueFromString(std::string& param, std::string& value) override;

    // This is called when caching is disabled because memory becomes sparse.
    void disableCaching() override;

    // This is called initially to learn parameter values from a random training
    // set.
    void learn() override;

    // Start the search engine to estimate the Q-value of a single action
    void estimateQValue(State const& state, int actionIndex,
                        double& qValue) override;

    // Start the search engine to estimate the Q-values of all applicable
    // actions
    void estimateQValues(State const& state,
                         std::vector<int> const& actionsToExpand,
                         std::vector<double>& qValues) override;

    // Parameter setter
    void setMaxSearchDepth(int _maxSearchDepth) override;
    void setCachingEnabled(bool newValue) override;

    void setStrictTerminationTimeout(double newValue) {
        strictTerminationTimeout = newValue;
    }

    void setTerminateWithReasonableAction(bool newValue) {
        terminateWithReasonableAction = newValue;
    }

    // Reset statistic variables
    void resetStats();

    // Print
    void printStats(std::ostream& out, bool const& printRoundStats,
                    std::string indent = "") const;

    // Caching
    typedef std::unordered_map<State, std::vector<double>,
                               State::HashWithoutRemSteps,
                               State::EqualWithoutRemSteps>
        HashMap;
    static HashMap rewardCache;

protected:
    // Decides whether more iterations are possible and reasonable
    bool moreIterations(int const& stepsToGo,
                        std::vector<int> const& actionsToExpand,
                        std::vector<double>& qValues);
    inline bool moreIterations(int const& stepsToGo);

    // The depth first search engine
    DepthFirstSearch* dfs;

    // Learning related variables
    bool isLearning;
    std::vector<std::vector<double>> elapsedTime;

    // Stopwatch used to make sure that computation doesn't take too much time
    Stopwatch stopwatch;

    // The number of remaining steps for this step
    int maxSearchDepthForThisStep;

    // Is true if caching was disabled at some point
    bool ramLimitReached;

    // Parameter
    double strictTerminationTimeout;
    bool terminateWithReasonableAction;

    // Statistics
    int accumulatedSearchDepth;
    int cacheHits;
    int numberOfRuns;
};

#endif
