#ifndef UNIFORM_EVALUATION_SEARCH_H
#define UNIFORM_EVALUATION_SEARCH_H

#include "search_engine.h"

// Evaluates all actions equally and can be used as a random search
// engine (standalone) or as a uniform initialization for algorithms
// in the THTS framework

class UniformEvaluationSearch : public DeterministicSearchEngine {
public:
    UniformEvaluationSearch();

    // Set parameters from command line
    bool setValueFromString(std::string& param, std::string& value) override;

    // Start the search engine to calculate best actions
    void estimateBestActions(State const& _rootState,
                             std::vector<int>& bestActions) override;

    // Start the search engine to estimate the Q-value of a single action
    void estimateQValue(State const& state, int /*actionIndex*/,
                        double& qValue) override {
        qValue = initialValue * (double)state.stepsToGo();
    }

    // Start the search engine to estimate the Q-values of all applicable
    // actions
    void estimateQValues(State const& state,
                         std::vector<int> const& actionsToExpand,
                         std::vector<double>& qValues) override;

    // Parameter setter
    virtual void setInitialValue(double const& _initialValue) {
        initialValue = _initialValue;
    }

private:
    // Parameter
    double initialValue;
};

#endif
