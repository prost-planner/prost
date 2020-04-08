#ifndef PROST_PLANNER_H
#define PROST_PLANNER_H

#include "search_engine.h"
#include "states.h"

#include <cassert>

class PlanningTask;

class ProstPlanner {
public:
    enum TimeoutManagementMethod { NONE, UNIFORM };

    ProstPlanner(std::string& plannerDesc);

    // Start or end the session
    void initSession(int _numberOfRounds, long totalTime);
    void finishSession(double& totalReward);

    // These are called at the beginning and end of a round
    void initRound(long const& remainingTime);
    void finishRound(double const& roundReward);

    // These are called at the beginning and end of a step
    void initStep(std::vector<double> const& nextStateVec,
                  long const& remainingTime);
    void finishStep(double const& immediateReward);

    // This is the main function of the PROST planner that starts the search
    // engine and return the next decision
    std::vector<std::string> plan();

    // Parameter setters
    void setSearchEngine(SearchEngine* _searchEngine) {
        searchEngine = _searchEngine;
    }

    void setSeed(int _seed);

    void setRAMLimit(int _ramLimit) {
        // Since we cannot guarantee that no further memory is allocated after
        // caching is disabled, we keep a buffer of 25% of the available memory
        // TODO: We should be able to guarantee this instead and use all
        //  available memory instead
        ramLimit = 0.75 * _ramLimit;
    }

    void setBitSize(int _bitSize) {
        bitSize = _bitSize;
    }

    void setTimeoutManagementMethod(TimeoutManagementMethod _tmMethod) {
        tmMethod = _tmMethod;
    }
    
    // Resets the static objects used within all components. Has to be called if
    // the planner is used multiple times within one run, or for unit tests
    static void resetStaticMembers();

private:
    // Checks how much memory is used and aborts caching if necessary
    void monitorRAMUsage();

    // Assigns a timeout for the next decision
    void manageTimeouts(long const& remainingTime);

    void printConfig() const;

    SearchEngine* searchEngine;

    State currentState;

    int currentRound;
    int currentStep;
    int stepsToGo;
    int executedActionIndex;
    int numberOfRounds;

    bool cachingEnabled;

    int remainingTimeFactor;

    // Parameter
    int ramLimit;
    int bitSize;
    int seed;
    TimeoutManagementMethod tmMethod;
};

#endif
