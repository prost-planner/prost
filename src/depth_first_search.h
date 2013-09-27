#ifndef DEPTH_FIRST_SEARCH_H
#define DEPTH_FIRST_SEARCH_H

// Implements a depth first search engine. Is currently only called
// from within IDS search.

#include "search_engine.h"

#include <set>
#include <cassert>

class ProstPlanner;
class UCTSearchEngine;

class DepthFirstSearch : public SearchEngine {
public:
    DepthFirstSearch(ProstPlanner* _planner);

    // Start the search engine for Q-value estimation
    bool estimateQValues(State const& _rootState, std::vector<int> const& actionsToExpand, std::vector<double>& qValues);

private:
    // Returns the reward that can be achieved if the action with
    // index actionIndex is applied to State state
    void applyAction(State const& state, int const& actionIndex, double& reward);

    // Expands State state and calculates the reward that can be
    // achieved by applying any action in that state
    void expandState(State const& state, double& res);

    double rewardHelperVar;
};

#endif
