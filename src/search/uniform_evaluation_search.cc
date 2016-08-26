#include "uniform_evaluation_search.h"

#include <limits>

using namespace std;

/******************************************************************
                     Search Engine Creation
******************************************************************/

UniformEvaluationSearch::UniformEvaluationSearch()
    : DeterministicSearchEngine("UniformEvaluation"), initialValue(0.0) {}

bool UniformEvaluationSearch::setValueFromString(string& param, string& value) {
    if (param == "-val") {
        if (value == "MAX") {
            setInitialValue(SearchEngine::rewardCPF->getMaxVal());
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

void UniformEvaluationSearch::estimateBestActions(
    State const& _rootState, std::vector<int>& bestActions) {
    // All applicable actions are equally good
    bestActions = getIndicesOfApplicableActions(_rootState);
}

void UniformEvaluationSearch::estimateQValues(
    State const& state, vector<int> const& actionsToExpand,
    vector<double>& qValues) {
    // Assign the initial value to all applicable actions
    for (unsigned int index = 0; index < qValues.size(); ++index) {
        if (actionsToExpand[index] == index) {
            qValues[index] = initialValue * (double)state.stepsToGo();
        }
    }
}
