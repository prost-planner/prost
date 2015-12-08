#ifndef ITERATIVE_DEEPENING_SEARCH_H
#define ITERATIVE_DEEPENING_SEARCH_H

// Implements an iterative deepending search engine. This was used as
// initialization for UCT in IPPC 2011 and is described in the paper
// by Keller and Eyerich (2012).

#include "search_engine.h"
#include "states.h"

#include "utils/timer.h"

#include <unordered_map>

class DepthFirstSearch;
class MinimalLookaheadSearch;

class IDS : public DeterministicSearchEngine {
public:
    IDS();

    // Set parameters from command line
    bool setValueFromString(std::string& param, std::string& value);

    // This is called when caching is disabled because memory becomes sparse.
    void disableCaching();

    // This is called initially to learn parameter values from a random training
    // set.
    void learn();

    // Start the search engine to estimate the Q-value of a single action
    bool estimateQValue(State const& state, int actionIndex,
                        double& qValue) override;
    
    // Start the search engine to estimate the Q-values of all applicable
    // actions
    bool estimateQValues(State const& state,
                         std::vector<int> const& actionsToExpand,
                         std::vector<double>& qValues) override;

    // Parameter setter
    void setMaxSearchDepth(int _maxSearchDepth);

    virtual void setStrictTerminationTimeout(double newValue) {
        strictTerminationTimeout = newValue;
    }

    virtual void setTerminateWithReasonableAction(bool newValue) {
        terminateWithReasonableAction = newValue;
    }

    virtual void setCachingEnabled(bool newValue);

    // Reset statistic variables
    void resetStats();

    // Print
    void printStats(std::ostream& out, bool const& printRoundStats,
                    std::string indent = "") const;

    // Caching
    typedef std::unordered_map<State, std::vector<double>,
                               State::HashWithoutRemSteps,
                               State::EqualWithoutRemSteps> HashMap;
    static HashMap rewardCache;

protected:
    // Decides whether more iterations are possible and reasonable
    bool moreIterations(std::vector<int> const& actionsToExpand,
                        std::vector<double>& qValues);
    inline bool moreIterations();

    // The state that is given iteratively to the DFS engine
    State currentState;

    // Learning related variables
    bool isLearning;
    std::vector<std::vector<double> > elapsedTime;

    // The timer that is used to make sure that computation doesn't take too
    // much time
    Timer timer;

    // The depth first search engine
    DepthFirstSearch* dfs;

    // The minimal lookahead search engine that is used if initialization with
    // this is too costly. TODO: In the future, we should change bool
    // estimateBestAction to SearchEngine* estimateBestAction and return another
    // search engine if this wants to be replaces
    MinimalLookaheadSearch* mls;

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
