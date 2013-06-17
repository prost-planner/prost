#ifndef RANDOM_SEARCH_H
#define RANDOM_SEARCH_H

#include "search_engine.h"

// Evaluates all actions equally and can be used as a random search
// engine (standalone) or as a uniform initialization for algorithms
// in the THTS framework

class UniformEvaluationSearch : public SearchEngine {
public:
    UniformEvaluationSearch(ProstPlanner* _planner);

    //main (public) search functions
    void estimateQValues(State const& _rootState, std::vector<double>& result, bool const& pruneResult);
};

#endif
