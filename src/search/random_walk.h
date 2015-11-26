#ifndef RANDOM_WALK_H
#define RANDOM_WALK_H

#include "search_engine.h"

// Evaluates all actions by simulating a run that starts with that action
// followed by random actions until a terminal state is reached

class RandomWalk : public ProbabilisticSearchEngine {
public:
    RandomWalk();

    // Set parameters from command line
    bool setValueFromString(std::string& param, std::string& value);

    // Start the search engine for Q-value estimation
    bool estimateQValues(State const& _rootState,
                         std::vector<int> const& actionsToExpand,
                         std::vector<double>& qValues);

    // Parameter Setter
    virtual void setNumberOfIterations(int _numberOfIterations) {
        numberOfIterations = _numberOfIterations;
    }

private:
    void performRandomWalks(PDState const& root, int firstActionIndex,
                            double& result) const;
    void sampleSuccessorState(PDState const& current, int const& actionIndex,
                              PDState& next, double& reward) const;

    // Parameter
    int numberOfIterations;
};

#endif
