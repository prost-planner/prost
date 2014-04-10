#ifndef PLANNING_TASK_H
#define PLANNING_TASK_H

#include "states.h"

#include <vector>
#include <map>
#include <list>

class StateFluent;
class Evaluatable;
class ConditionalProbabilityFunction;
class RewardFunction;

struct PlanningTask {
    enum FinalRewardCalculationMethod {
        NOOP,
        FIRST_APPLICABLE,
        BEST_OF_CANDIDATE_SET
    };

    // The name of this task (this is equivalent to the instance name)
    static std::string name;

    // Random set of reachable states (these are used for learning)
    static std::vector<State> trainingSet;

    //void determinePruningEquivalence();

    // Action fluents and action states
    static std::vector<ActionFluent*> actionFluents;
    static std::vector<ActionState> actionStates;

    // State fluents
    static std::vector<StateFluent*> stateFluents;

    // The CPFs
    static std::vector<ConditionalProbabilityFunction*> CPFs;

    // The reward formula
    static RewardFunction* rewardCPF;

    // The action preconditions
    static std::vector<Evaluatable*> actionPreconditions;

    // Is true if this planning task is deterministic
    static bool isDeterministic;

    // Is true if this task's determinization is equivalent w.r.t. to
    // reasonable action pruning
    // bool isPruningEquivalentToDet;

    // The initial state (the one given in the problem description,
    // this is not updated)
    static State initialState;

    // The problem horizon
    static int horizon;

    // The problem's discount factor (TODO: This is ignored at the moment)
    static double discountFactor;

    // The number of actions (this is equal to actionStates.size())
    static int numberOfActions;

    // The index of the first probabilistic variable (variables are
    // ordered s.t. all deterministic ones come first)
    static int firstProbabilisticVarIndex;

    // Since the reward is independent from the successor state, we can
    // calculate the final reward as the maximum of applying all actions in the
    // current state. Since the set of actions that could maximize the final
    // reward can be a subset of all actions, we distinguish between several
    // different methods to speed up this calculation.
    static FinalRewardCalculationMethod finalRewardCalculationMethod;
    static std::vector<int> candidatesForOptimalFinalAction;

    // Printer functions
    static void print(std::ostream& out);

    static void printEvaluatableInDetail(std::ostream& out, Evaluatable* eval);
    static void printCPFInDetail(std::ostream& out, int const& index);
    static void printRewardCPFInDetail(std::ostream& out);
    static void printActionPreconditionInDetail(std::ostream& out, int const& index);

private:
    PlanningTask() {}
};

#endif
