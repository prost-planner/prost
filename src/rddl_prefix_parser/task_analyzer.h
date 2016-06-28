#ifndef TASK_ANALYZER_H
#define TASK_ANALYZER_H

#include "states.h"

#include <vector>
#include <set>

class PlanningTask;

class TaskAnalyzer {
public:
    TaskAnalyzer(PlanningTask* _task) :
        task(_task) {}

    void analyzeTask(int const& numberOfStates = 200,
            double const& maxTimeout = 2.0);

protected:
    PlanningTask* task;

    std::set<State, State::StateSort> encounteredStates;

    void analyzeStateAndApplyAction(State const& current, State& next, double& reward) const;

    bool actionIsApplicable(ActionState const& action, State const& current) const;

    void detectUnreasonableActionsInDeterminization(State const& current) const;

    bool isARewardLock(State const& current, double const& reward) const;
    bool checkDeadEnd(KleeneState const& state) const;
    bool checkGoal(KleeneState const& state) const;

    void createTrainingSet(int const& numberOfStates);
};

#endif
