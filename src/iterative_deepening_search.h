#ifndef ITERATIVE_DEEPENING_SEARCH_H
#define ITERATIVE_DEEPENING_SEARCH_H

#include "search_engine.h"

#include "utils/timer.h"

#include <map>

class ProstPlanner;
class DepthFirstSearch;

class IterativeDeepeningSearch : public SearchEngine {
public:
    IterativeDeepeningSearch(ProstPlanner* _planner, std::vector<double>& _result);

    bool setValueFromString(std::string& param, std::string& value);

    void learn(std::vector<State> const& trainingSet);

    ////parameter setters: overwrites
    void setResultType(SearchEngine::ResultType const _resultType);
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
        if(task->noopIsOptimalFinalAction()) {
            ++minSearchDepth;
        }
    }

    virtual void setCachingEnabled(bool _cachingEnabled) {
        SearchEngine::setCachingEnabled(_cachingEnabled);
        dfs->setCachingEnabled(_cachingEnabled);
    }

    void resetStats();
    void printStats(std::string indent = "");

protected:
    void _run();
    bool moreIterations();

    //learning related stuff
    bool isLearning;
    std::vector<std::vector<double> > elapsedTime;

    //timer related stuff
    Timer timer;
    double time;

    //the used depth first search and the vector where dfs writes the result
    SearchEngine* dfs;
    std::vector<double> dfsResult;

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
