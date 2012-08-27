#ifndef RANDOM_SEARCH_H
#define RANDOM_SEARCH_H

#include "search_engine.h"

//evaluates all actions equally and is used if no initialization is desired in UCTSearch

class RandomSearch : public SearchEngine {
public:
    RandomSearch(ProstPlanner* _planner, std::vector<double>& _result);

protected:
    void _run();
};

#endif
