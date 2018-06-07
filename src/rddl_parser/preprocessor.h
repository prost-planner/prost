#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

#include <list>
#include <map>
#include <set>
#include <string>
#include <vector>

class RDDLTask;
class Evaluatable;
class ActionPrecondition;
class ActionState;
class State;
class StateFluent;

class Preprocessor {
public:
    Preprocessor(RDDLTask* task, bool useIPC2018Rules)
        : task(task), useIPC2018Rules(useIPC2018Rules) {}

    void preprocess(bool const& output = true);

private:
    RDDLTask* task;
    bool useIPC2018Rules;

    void prepareEvaluatables();
    void prepareActions();
    void removeInapplicableActionFluents(bool const& updateActionStates);
    void initializeActionStates();

    void calcAllActionStates(std::vector<ActionState>& result,
                             int minElement, int scheduledActions) const;
    void calcAllActionStatesForIPC2018(std::vector<ActionState>& base,
                                       std::set<ActionState>& result) const; 

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
