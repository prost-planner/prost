#ifndef TASK_ANALYZER_H
#define TASK_ANALYZER_H

#include "states.h"

#include <set>
#include <vector>

namespace prost {
namespace parser {
struct RDDLTask;

class TaskAnalyzer {
public:
    explicit TaskAnalyzer(RDDLTask* _task) : task(_task) {}

    void analyzeTask(
        int numStates, int numSimulations, double timeout, bool output = true);

protected:
    RDDLTask* task;

    std::set<State, State::StateSort> encounteredStates;
    bool rewardFormulaAllowsRewardLockDetection;

    void determineTaskProperties();

    void calculateMinAndMaxReward() const;

    void analyzeStateAndApplyAction(State const& current, State& next,
                                    double& reward) const;

    bool actionIsApplicable(ActionState const& action,
                            State const& current) const;

    void detectUnreasonableActionsInDeterminization(State const& current) const;

    bool isARewardLock(State const& current, double const& reward) const;
    bool checkDeadEnd(KleeneState const& state) const;
    bool checkGoal(KleeneState const& state) const;

    void createTrainingSet(int const& numberOfStates);
};
} // namespace parser
} // namespace prost

#endif
