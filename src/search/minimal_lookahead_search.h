#ifndef MINIMAL_LOOKAHEAD_SEARCH_H
#define MINIMAL_LOOKAHEAD_SEARCH_H

#include "search_engine.h"

#include <unordered_map>

class MinimalLookaheadSearch : public DeterministicSearchEngine {
public:
    MinimalLookaheadSearch();

    // Notify the search engine that a new round starts
    void initRound() override {
        numberOfRunsInCurrentRound = 0;
    }

    // Notify the search engine that a new step starts or ends
    void initStep(State const& /*current*/) override {
        numberOfRuns = 0;
        cacheHits = 0;
    }
    void finishStep() override {
        numberOfRunsInCurrentRound += numberOfRuns;
    }

    // Start the search engine to estimate the Q-value of a single action
    void estimateQValue(State const& state, int actionIndex,
                        double& qValue) override;

    // Start the search engine to estimate the Q-values of all applicable
    // actions
    void estimateQValues(State const& state,
                         std::vector<int> const& actionsToExpand,
                         std::vector<double>& qValues) override;

    bool usesBDDs() const override {
        return false;
    }

    // Print
    void printRoundStatistics(std::string indent) const override;
    void printStepStatistics(std::string indent) const override;

    // Caching
    typedef std::unordered_map<State, std::vector<double>,
                               State::HashWithoutRemSteps,
                               State::EqualWithoutRemSteps>
        HashMap;
    static HashMap rewardCache;

protected:
    void printRewardCacheUsage(
            std::string indent, Verbosity verbosity = Verbosity::VERBOSE) const;

    // Per step statistics
    int numberOfRuns;
    int cacheHits;

    // Per round statistics
    int numberOfRunsInCurrentRound;
};

#endif
