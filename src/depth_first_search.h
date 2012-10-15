#ifndef DEPTH_FIRST_SEARCH_H
#define DEPTH_FIRST_SEARCH_H

#include "search_engine.h"

#include <set>
#include <cassert>

class ProstPlanner;
class UCTSearchEngine;

class DepthFirstSearch : public SearchEngine {
public:
    DepthFirstSearch(ProstPlanner* _planner);

    //main (public) search functions
    void estimateQValues(State const& _rootState, std::vector<double>& result, const bool& pruneResult);
    void estimateBestActions(State const& /*_rootState*/, std::vector<int>& /*result*/){
        //TODO: This is not needed currently but might be in the future (and is quite straightforward), so implement it
        assert(false);
    }

private:
    void applyAction(State const& state, int const& actionIndex, double& reward);
    void expandState(State const& state, double& res);

    double rewardHelperVar;
};

#endif
