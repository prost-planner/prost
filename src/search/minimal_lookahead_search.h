#ifndef MINIMAL_LOOKAHEAD_SEARCH_H
#define MINIMAL_LOOKAHEAD_SEARCH_H

#include "search_engine.h"

#include <unordered_map>

class MinimalLookaheadSearch : public DeterministicSearchEngine {
public:
    MinimalLookaheadSearch();

    // Start the search engine to estimate the Q-value of a single action
    void estimateQValue(State const& state, int actionIndex,
                        double& qValue) override;

    // Start the search engine to estimate the Q-values of all applicable
    // actions
    void estimateQValues(State const& state,
                         std::vector<int> const& actionsToExpand,
                         std::vector<double>& qValues) override;

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
    // Statistics
    int numberOfRuns;
    int cacheHits;
};

#endif
