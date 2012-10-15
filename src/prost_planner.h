#ifndef PROST_PLANNER_H
#define PROST_PLANNER_H

#include "search_engine.h"

#include <map>

class UnprocessedPlanningTask;
class PlanningTask;
class CachingComponent;
class LearningComponent;

class ProstPlanner {
public:
    static ProstPlanner* fromString(std::string& desc, std::string& domain, std::string& problem, int& numberOfRounds);

    ProstPlanner(std::string domain, std::string problem, int _numberOfRounds);

    void init(std::map<std::string,int>& stateVariableIndices);
    void initNextRound();
    std::vector<std::string> plan(std::vector<double> const& nextStateVec);

    //all components that use caches register here so that caching can be switched off if memory becomes sparse
    void registerCachingComponent(CachingComponent* component) {
        cachingComponents.push_back(component);
    }
    void unregisterCachingComponent(CachingComponent* component);

    //all components that learn initially register here
    void registerLearningComponent(LearningComponent* component) {
        learningComponents.push_back(component);
    }
    void unregisterLearningComponent(LearningComponent* component);

    //parameter setters
    void setSearchEngine(SearchEngine* _searchEngine) {
        searchEngine = _searchEngine;
    }

    void setSeed(int _seed);

    void setNumberOfRounds(int _numberOfRounds) {
        numberOfRounds = _numberOfRounds;
    }

    void setRAMLimit(int _ramLimit) {
        ramLimit = _ramLimit;
    }

    void setBitSize(int _bitSize) {
        bitSize = _bitSize;
    }

    //getters for global variables and parameters
    PlanningTask* getProbabilisticTask() {
        assert(probabilisticTask != NULL);
        return probabilisticTask;
    }

    PlanningTask* getDeterministicTask() {
        assert(deterministicTask != NULL);
        return deterministicTask;
    }

    int const& getNumberOfRounds() const {
        return numberOfRounds;
    }

    int const& getRAMLimit() const {
        return ramLimit;
    }

    int const& getBitSize() const {
        return bitSize;
    }

private:
    void manageTimeouts();
    int combineResults();

    void initNextStep(std::vector<double> const& nextStateVec);

    void monitorRAMUsage();

    void printStep(int result, bool printSearchEngineLogs = true);

    UnprocessedPlanningTask* unprocessedTask;
    PlanningTask* probabilisticTask;
    PlanningTask* deterministicTask;

    std::string searchEngineDesc;
    SearchEngine* searchEngine;

    State currentState;

    int currentRound;
    int remainingSteps;
    int numberOfRounds;

    bool cachingEnabled;

    int ramLimit;
    int bitSize;
    int seed;

    int chosenActionIndex;

    std::vector<int> bestActions;

    std::vector<CachingComponent*> cachingComponents;
    std::vector<LearningComponent*> learningComponents;
};

#endif
