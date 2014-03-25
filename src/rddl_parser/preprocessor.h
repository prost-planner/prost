#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

#include <list>
#include <map>
#include <string>
#include <vector>

class PlanningTask;
class ActionPrecondition;
class ActionState;

class Preprocessor {
public:
    Preprocessor(PlanningTask* _task) :
        task(_task) {}

    void preprocess();

private:
    PlanningTask* task;

    void prepareEvaluatables();
    void prepareActions();
    void calcPossiblyLegalActionStates(int actionsToSchedule,
                                       std::list<std::vector<int> >& result,
                                       std::vector<int> addTo = std::vector<int>()) const;
    bool sacContainsNegativeActionFluent(ActionPrecondition* const& sac, ActionState const& actionState) const;
    bool sacContainsAdditionalPositiveActionFluent(ActionPrecondition* const& sac, ActionState const& actionState) const;
    void calculateDomains();
    void finalizeEvaluatables();
    void determinize();
    void determineTaskProperties();

    void prepareStateHashKeys();
    void prepareKleeneStateHashKeys();
    void prepareStateFluentHashKeys();
};

#endif