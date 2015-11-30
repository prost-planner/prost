#ifndef MINIMAL_LOOKAHEAD_SEARCH_H
#define MINIMAL_LOOKAHEAD_SEARCH_H

#include "search_engine.h"

#include <unordered_map>

class MinimalLookaheadSearch : public DeterministicSearchEngine {
public:
    MinimalLookaheadSearch();

    // Start the search engine for Q-value estimation
    bool estimateQValues(State const& current,
                         std::vector<int> const& actionsToExpand,
                         std::vector<double>& qValues);

    // Print
    void printStats(std::ostream& out, bool const& printRoundStats,
                    std::string indent = "") const;

    // Caching
    typedef std::unordered_map<State, std::vector<double>, 
                               State::HashWithoutRemSteps, 
                               State::EqualWithoutRemSteps> HashMap;
    static HashMap rewardCache;

protected:
    
    // Statistics
    int numberOfRuns;
    int cacheHits;
};

#endif
