#include "random_search.h"

#include "prost_planner.h"

#include <limits>

using namespace std;

RandomSearch::RandomSearch(ProstPlanner* _planner, vector<double>& _result) :
    SearchEngine("Random", _planner, _planner->getDeterministicTask(), _result) {}

void RandomSearch::_run() {
    switch(resultType) {
    case SearchEngine::ESTIMATE:
        for(unsigned int i = 0; i < result.size(); ++i) {
            result[i] = 0.0;
        }
        break;
    case SearchEngine::PRUNED_ESTIMATE:
        task->setActionsToExpand(rootState, actionsToExpand);
        for(unsigned int i = 0; i < result.size(); ++i) {
            if(actionsToExpand[i] == i) {
                result[i] = 0.0;
            } else {
                result[i] = -std::numeric_limits<double>::max();
            }
        }
        break;
    default:
        assert(false);
        break;
    }
}


