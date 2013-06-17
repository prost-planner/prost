#include "uniform_evaluation_search.h"

#include "prost_planner.h"

#include <limits>

using namespace std;

UniformEvaluationSearch::UniformEvaluationSearch(ProstPlanner* _planner) :
    SearchEngine("UniformEvaluation", _planner, _planner->getDeterministicTask()),
    initialValue(0.0) {}

bool UniformEvaluationSearch::setValueFromString(string& param, string& value) {
    if(param == "-val") {
        if(value == "INFTY") {
            setInitialValue(task->getMaxReward());
        } else {
            setInitialValue(atof(value.c_str()));
        }
        return true;
    }

    return SearchEngine::setValueFromString(param, value);
}

void UniformEvaluationSearch::estimateQValues(State const& _rootState, vector<double>& result, bool const& pruneResult) {
    vector<int> actionsToExpand = task->getApplicableActions(_rootState, pruneResult);

    for(unsigned int i = 0; i < result.size(); ++i) {
        if(actionsToExpand[i] == i) {
            result[i] = initialValue;
        } else {
            result[i] = -std::numeric_limits<double>::max();
        }
    }
}
