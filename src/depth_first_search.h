#ifndef DEPTH_FIRST_SEARCH_H
#define DEPTH_FIRST_SEARCH_H

#include "search_engine.h"

#include <set>
#include <cassert>

class ProstPlanner;
class UCTSearchEngine;

class DepthFirstSearch : public SearchEngine {
public:
    DepthFirstSearch(ProstPlanner* _planner, std::vector<double>& _result);

    int getNumberOfEvaluatedStates();
    int getNumberOfCacheHits();

private:
    void _run();

    void expandState(State& cur, std::vector<double>& res);
    void applyAction(State& state, int actionIndex, double& reward);

    int evaluatedStates;
    int cacheHits;
    int bestRewardIndex;
    double rewardHelperVar;
    double referenceReward;
};

#endif
