#include "uniform_evaluation_search.h"

#include "prost_planner.h"

#include <limits>

using namespace std;

UniformEvaluationSearch::UniformEvaluationSearch(ProstPlanner* _planner) :
    SearchEngine("UniformEvaluation", _planner, _planner->getDeterministicTask()) {}

void UniformEvaluationSearch::estimateQValues(State const& _rootState, vector<double>& result, bool const& pruneResult) {
    vector<int> actionsToExpand = task->getApplicableActions(_rootState, pruneResult);

    for(unsigned int i = 0; i < result.size(); ++i) {
        if(actionsToExpand[i] == i) {
            result[i] = 0.0;
        } else {
            result[i] = -std::numeric_limits<double>::max();
        }
    }
}
