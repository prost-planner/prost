#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

#include <list>
#include <map>
#include <string>
#include <vector>

class PlanningTask;
class Evaluatable;
class ActionPrecondition;
class ActionState;
class State;
class StateFluent;

class Preprocessor {
public:
    Preprocessor(PlanningTask* _task) : task(_task) {}

    void preprocess(bool const& output = true);

private:
    PlanningTask* task;

    void prepareEvaluatables();
    void prepareActions();
    void removeInapplicableActionFluents(bool const& updateActionStates);
    void initializeActionStates();

    void calcAllActionStates(std::vector<ActionState>& result,
                             int minElement = 0,
                             int scheduledActions = 0) const;

    bool sacContainsNegativeActionFluent(ActionPrecondition* const& sac,
                                         ActionState const& actionState) const;
    bool sacContainsAdditionalPositiveActionFluent(
        ActionPrecondition* const& sac, ActionState const& actionState) const;

    void calculateCPFDomains();
    void finalizeEvaluatables();
    void determinize();

    void determineTaskProperties();
    bool actionStateIsDominated(int stateIndex) const;
    bool actionStateDominates(ActionState const& lhs,
                              ActionState const& rhs) const;
    void addDominantState(int stateIndex) const;

    void prepareStateHashKeys();
    void prepareKleeneStateHashKeys();
    void prepareStateFluentHashKeys();

    void precomputeEvaluatables();
    void precomputeEvaluatable(Evaluatable* eval);
    void createRelevantStates(std::vector<StateFluent*>& dependentStateFluents,
                              std::vector<State>& result);
    long calculateStateFluentHashKey(Evaluatable* eval,
                                     State const& state) const;

    void calculateMinAndMaxReward() const;
};

#endif
