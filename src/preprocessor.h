#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

#include <list>
#include <map>
#include <string>
#include <vector>

class UnprocessedPlanningTask;
class PlanningTask;
class StateFluent;
class NumericConstant;
class Evaluatable;
class ConditionalProbabilityFunction;
class RewardFunction;
class ActionFluent;
class ActionState;

class Preprocessor {
public:
    Preprocessor(UnprocessedPlanningTask* _task) :
        task(_task) {}

    PlanningTask* preprocess(std::map<std::string,int>& stateVariableIndices, std::vector<std::vector<std::string> >& stateVariableValues);

private:
    UnprocessedPlanningTask* task;

    void prepareEvaluatables();
    void prepareActions();
    void calcPossiblyLegalActionStates(int actionsToSchedule,
                                       std::list<std::vector<int> >& result,
                                       std::vector<int> addTo = std::vector<int>()) const;
    bool sacContainsNegativeActionFluent(Evaluatable* const& sac, ActionState const& actionState) const;
    bool sacContainsAdditionalPositiveActionFluent(Evaluatable* const& sac, ActionState const& actionState) const;
    void calculateDomains();
    void finalizeEvaluatables();
    void determinize();
    void determineTaskProperties();

    void prepareStateHashKeys();
    void prepareKleeneStateHashKeys();
    void prepareStateFluentHashKeys();
};

#endif
