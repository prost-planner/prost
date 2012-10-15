#include "random_search.h"

#include "prost_planner.h"

#include <limits>

using namespace std;

RandomSearch::RandomSearch(ProstPlanner* _planner) :
    SearchEngine("Random", _planner, _planner->getDeterministicTask()) {}

void RandomSearch::estimateQValues(State const& _rootState, vector<double>& result, bool const& pruneResult) {
    if(pruneResult) {
        vector<int> actionsToExpand(task->getNumberOfActions(),-1);
        task->setActionsToExpand(_rootState, actionsToExpand);
        for(unsigned int i = 0; i < result.size(); ++i) {
            if(actionsToExpand[i] == i) {
                result[i] = 0.0;
            } else {
                result[i] = -std::numeric_limits<double>::max();
            }
        }
    } else {
        for(unsigned int i = 0; i < result.size(); ++i) {
            result[i] = 0.0;
        }
    }
}
