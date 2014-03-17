#include "uniform_evaluation_search.h"

#include "prost_planner.h"
#include "planning_task.h"

#include <limits>

using namespace std;

/******************************************************************
                     Search Engine Creation
******************************************************************/

UniformEvaluationSearch::UniformEvaluationSearch(ProstPlanner* _planner, PlanningTask* _task) :
    SearchEngine("UniformEvaluation", _planner, _task, true),
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

/******************************************************************
                       Main Search Functions
******************************************************************/

bool UniformEvaluationSearch::estimateBestActions(State const& _rootState, std::vector<int>& bestActions) {
    bestActions = getIndicesOfApplicableActions(_rootState);
    return true;
}

bool UniformEvaluationSearch::estimateQValues(State const& /*_rootState*/, vector<int> const& actionsToExpand, vector<double>& qValues) {
    for(unsigned int actionIndex = 0; actionIndex < qValues.size(); ++actionIndex) {
        if(actionsToExpand[actionIndex] == actionIndex) {
            qValues[actionIndex] = initialValue;
        }
    }
    return true;
}
