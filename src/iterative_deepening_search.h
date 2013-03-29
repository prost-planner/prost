#ifndef ITERATIVE_DEEPENING_SEARCH_H
#define ITERATIVE_DEEPENING_SEARCH_H

#include "search_engine.h"

#include "utils/timer.h"

#include <map>

class ProstPlanner;
class DepthFirstSearch;

class IterativeDeepeningSearch : public SearchEngine {
public:
    //search engine creation
    IterativeDeepeningSearch(ProstPlanner* _planner);

    bool setValueFromString(std::string& param, std::string& value);

    //learning
    bool learn(std::vector<State> const& trainingSet);

    //main (public) search functions
    void estimateQValues(State const& _rootState, std::vector<double>& result, bool const& pruneResult);
    void estimateBestActions(State const& /*_rootState*/, std::vector<int>& /*result*/) {
        assert(false);
    }

    //parameter setters: overwrites
    void setMaxSearchDepth(int _maxSearchDepth);

    //parameter setters: new parameters
    virtual void setTerminationTimeout(double _terminationTimeout) {
        terminationTimeout = _terminationTimeout;
    }

    virtual void setStrictTerminationTimeout(double _strictTerminationTimeout) {
        strictTerminationTimeout = _strictTerminationTimeout;
    }

    virtual void setTerminateWithReasonableAction(bool _terminateWithReasonableAction) {
        terminateWithReasonableAction = _terminateWithReasonableAction;
    }

    virtual void setMinSearchDepth(int _minSearchDepth) {
        minSearchDepth = _minSearchDepth;
    }

    virtual void setCachingEnabled(bool _cachingEnabled) {
        SearchEngine::setCachingEnabled(_cachingEnabled);
        dfs->setCachingEnabled(_cachingEnabled);
    }

    //printing and statistics
    void resetStats();
    void printStats(std::ostream& out, std::string indent = "");

protected:
    bool moreIterations(std::vector<double>& result);

    State currentState;

    //learning related stuff
    bool isLearning;
    std::vector<std::vector<double> > elapsedTime;

    //timer related stuff
    Timer timer;
    double time;

    //the used depth first search engine
    SearchEngine* dfs;

    //The number of remaining steps that was queried
    int maxSearchDepthForThisStep;

    //parameter
    double terminationTimeout;
    double strictTerminationTimeout;
    bool terminateWithReasonableAction;
    int minSearchDepth;

    //statistics
    int accumulatedSearchDepth;
    int cacheHits;
    int numberOfRuns;

    //caching
    static std::map<State, std::vector<double>, State::CompareIgnoringRemainingSteps> rewardCache;
};

#endif
