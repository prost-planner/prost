#ifndef PROST_PLANNER_H
#define PROST_PLANNER_H

#include "search_engine.h"
#include "states.h"

#include <cassert>

class PlanningTask;

class ProstPlanner {
public:
    ProstPlanner(std::string& plannerDesc);

    void init();
    void initNextRound();
    std::vector<std::string> plan(std::vector<double> const& nextStateVec);

    // Parameter setters
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

    // Getters for global variables and parameters
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

    // Creates the current state according to values of nextStateVec and resets
    // the best actions
    void initNextStep(std::vector<double> const& nextStateVec);

    void monitorRAMUsage();

    void printStep(int result, bool printSearchEngineLogs = true);

    SearchEngine* searchEngine;

    State currentState;

    int currentRound;
    int remainingSteps;
    int numberOfRounds;

    bool cachingEnabled;

    int ramLimit;
    int bitSize;
    int seed;

    std::vector<int> bestActions;
};

#endif
