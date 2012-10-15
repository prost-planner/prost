#ifndef RANDOM_SEARCH_H
#define RANDOM_SEARCH_H

#include "search_engine.h"

//evaluates all actions equally and is used if no initialization is desired in UCTSearch

class RandomSearch : public SearchEngine {
public:
    RandomSearch(ProstPlanner* _planner);

    //main (public) search functions
    void estimateQValues(State const& _rootState, std::vector<double>& result, bool const& pruneResult);
    void estimateBestActions(State const& /*_rootState*/, std::vector<int>& /*result*/) {
        //TODO: This is not needed currently but might be in the future (and is quite straightforward), so implement it
        assert(false);
    }
};

#endif
