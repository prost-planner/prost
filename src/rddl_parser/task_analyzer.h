#ifndef PARSER_TASK_ANALYZER_H
#define PARSER_TASK_ANALYZER_H

/*
  The TaskAnalyzer determines some properties of the planning task.

  1. The reward formula does not allow "primed" state variables,  i.e., it does
     not depend on the successor state of an action application. We compute
     dominance between actions here that determines if some action is guaranteed
     to yield an immediate reward that is not smaller than the immediate reward
     of another action. If this is the case, the dominated action doesn't need
     to be applied in the final step.
  2. We determine the min and max reward of the reward function. If the reward
     function is precomputed (by the Precomputer class), this is
     straightforward. Otherwise, a function based on interval arithmetic is
     applied that is guaranteed to overapproximate the maximal reward and
     underapproximate the minimal reward.
  3. The main part of this class is a simulation of runs that is used to
     - check if there are unreasonable actions
     - checks if there are goals or dead ends
     - creates a sample set of states
*/

#include "states.h"

#include <set>
#include <vector>

namespace prost::parser {
struct RDDLTask;

class TaskAnalyzer {
public:
    explicit TaskAnalyzer(RDDLTask* _task) : task(_task) {}

    void analyzeTask(int numStates, int numSimulations, double timeout,
                     bool output = true);

protected:
    RDDLTask* task;

    std::set<State, State::StateSort> encounteredStates;
    int finalActionIndex = -1;

    void determineTaskProperties();
    std::vector<int> determineDominantActions() const;

    void calculateMinAndMaxReward() const;

    void performRandomWalks(int numSimulations, double timeout);

    void analyzeStateAndApplyAction(State const& current, State& next,
                                    double& reward) const;

    bool actionIsApplicable(ActionState const& action,
                            State const& current) const;

    void detectUnreasonableActionsInDeterminization(State const& current) const;

    bool isARewardLock(State const& current, double const& reward) const;
    bool checkDeadEnd(KleeneState const& state) const;
    bool checkGoal(KleeneState const& state) const;

    void createTrainingSet(int const& numberOfStates);

    void stateWithoutApplicableActionsDetected(State const& state) const;
};
} // namespace prost::parser

#endif // PARSER_TASK_ANALYZER_H
