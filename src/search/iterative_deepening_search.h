#ifndef ITERATIVE_DEEPENING_SEARCH_H
#define ITERATIVE_DEEPENING_SEARCH_H

// Implements an iterative deepening search engine. This was used as
// initialization for UCT in IPC 2011 (and IPC 2014) and is described in the
// paper by Keller and Eyerich (ICAPS 2012).

#include "search_engine.h"
#include "states.h"

#include "utils/stopwatch.h"

#include <unordered_map>
#include <logger.h>

class DepthFirstSearch;
class MinimalLookaheadSearch;

class IDS : public DeterministicSearchEngine {
public:
    IDS();

    // Set parameters from command line
    bool setValueFromString(std::string& param, std::string& value) override;

    // This is called when caching is disabled because memory becomes sparse.
    void disableCaching() override;

    // Notify the search engine that the session starts
    void initSession() override;

    // Notify the search engine that a new round starts or ends
    void initRound() override;
    void finishRound() override;

    // Notify the search engine that a new step starts or ends
    void initStep(State const& current) override;
    void finishStep() override;

    // Start the search engine to estimate the Q-value of a single action
    void estimateQValue(State const& state, int actionIndex,
                        double& qValue) override;

    // Start the search engine to estimate the Q-values of all applicable
    // actions
    void estimateQValues(State const& state,
                         std::vector<int> const& actionsToExpand,
                         std::vector<double>& qValues) override;

    // Parameter setter
    void setMaxSearchDepth(int _maxSearchDepth) override;
    void setCachingEnabled(bool newValue) override;

    void setStrictTerminationTimeout(double newValue) {
        strictTerminationTimeout = newValue;
    }

    void setTerminateWithReasonableAction(bool newValue) {
        terminateWithReasonableAction = newValue;
    }

    bool usesBDDs() const override {
        return false;
    }

    // Print
    void printConfig(std::string indent) const override;
    void printRoundStatistics(std::string indent) const override;
    void printStepStatistics(std::string indent) const override;

    // Caching
    using HashMap = std::unordered_map<State, std::vector<double>,
                                       State::HashWithoutRemSteps,
                                       State::EqualWithoutRemSteps>;
    static HashMap rewardCache;

protected:
    // Decides whether more iterations are possible and reasonable
    bool moreIterations(int const& stepsToGo,
                        std::vector<int> const& actionsToExpand,
                        std::vector<double>& qValues);
    inline bool moreIterations(int const& stepsToGo);

    void createMinimalLookaheadSearch();

    void printRewardCacheUsage(
        std::string indent, Verbosity verbosity = Verbosity::VERBOSE) const;

    // The depth first search engine
    DepthFirstSearch* dfs;

    // If maxSearchDepth is no larger than 1, a MinimalLookahead search engine
    // is faster and better informed, so this one is used
    MinimalLookaheadSearch* mlh;

    // Learning related variables
    bool isLearning;
    std::vector<std::vector<double>> elapsedTime;

    // Stopwatch used to make sure that computation doesn't take too much time
    Stopwatch stopwatch;

    // The number of remaining steps for this step
    int maxSearchDepthForThisStep;

    // Is true if caching was disabled at some point
    bool ramLimitReached;

    // Is true if the current step is in the task's initial state.
    bool isInitialState;

    // Parameter
    double strictTerminationTimeout;
    bool terminateWithReasonableAction;

    // Per step statistics
    int accumulatedSearchDepthInCurrentStep;
    int numberOfRunsInCurrentStep;
    int cacheHitsInCurrentStep;

    // Per round statistics
    double avgSearchDepthInInitialState;
    long accumulatedSearchDepthInCurrentRound;
    int numberOfRunsInCurrentRound;
};

#endif
