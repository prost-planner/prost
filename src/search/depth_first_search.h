#ifndef DEPTH_FIRST_SEARCH_H
#define DEPTH_FIRST_SEARCH_H

// Implements a depth first search engine on the determinized task. Is currently
// only called from within IDS search.

#include "search_engine.h"

#include <cassert>
#include <set>

class ProstPlanner;
class UCTSearchEngine;

class DepthFirstSearch : public DeterministicSearchEngine {
public:
    DepthFirstSearch();

    // Start the search engine to estimate the Q-value of a single action
    void estimateQValue(State const& state, int actionIndex,
                        double& qValue) override;

    // Start the search engine to estimate the Q-values of all applicable
    // actions
    void estimateQValues(State const& state,
                         std::vector<int> const& actionsToExpand,
                         std::vector<double>& qValues) override;

private:
    // Returns the reward that can be achieved if the action with
    // index actionIndex is applied to State state
    void applyAction(State const& state, int const& actionIndex,
                     double& reward);

    // Expands State state and calculates the reward that can be
    // achieved by applying any action in that state
    void expandState(State const& state, double& res);

    double rewardHelperVar;
};

#endif
