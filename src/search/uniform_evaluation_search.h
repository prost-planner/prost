#ifndef RANDOM_SEARCH_H
#define RANDOM_SEARCH_H

#include "search_engine.h"

// Evaluates all actions equally and can be used as a random search
// engine (standalone) or as a uniform initialization for algorithms
// in the THTS framework

class UniformEvaluationSearch : public DeterministicSearchEngine {
public:
    UniformEvaluationSearch();

    // Set parameters from command line
    bool setValueFromString(std::string& param, std::string& value);

    // Start the search engine to calculate best actions
    bool estimateBestActions(State const& _rootState,
            std::vector<int>& bestActions);

    // Start the search engine for state value estimation
    bool estimateStateValue(State const& /*_rootState*/, double& stateValue) {
        stateValue = initialValue;
        return true;
    }

    // Start the search engine for Q-value estimation
    bool estimateQValues(State const& _rootState,
            std::vector<int> const& actionsToExpand,
            std::vector<double>& qValues);

    // Parameter setter
    virtual void setInitialValue(double const& _initialValue) {
        initialValue = _initialValue;
    }

private:
    // Parameter
    double initialValue;
};

#endif
