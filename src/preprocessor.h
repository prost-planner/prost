#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

#include <list>
#include <map>
#include <string>
#include <vector>

class ProstPlanner;
class UnprocessedPlanningTask;
class PlanningTask;
class StateFluent;
class NumericConstant;
class Evaluatable;
class ConditionalProbabilityFunction;
class RewardFunction;
class State;
class ActionFluent;
class ActionState;

class Preprocessor {
public:
    Preprocessor(ProstPlanner* _planner, UnprocessedPlanningTask* _task, PlanningTask* _probPlanningTask) :
        planner(_planner), task(_task), probPlanningTask(_probPlanningTask) {}

    PlanningTask* preprocess(std::map<std::string,int>& stateVariableIndices, std::vector<std::vector<std::string> >& stateVariableValues);

private:
    ProstPlanner* planner;
    UnprocessedPlanningTask* task;
    PlanningTask* probPlanningTask;

    void prepareSACs(std::vector<Evaluatable*>& dynamicSACs,
                     std::vector<Evaluatable*>& staticSACs,
                     std::vector<Evaluatable*>& stateInvariants);
    void prepareCPFs(std::vector<ConditionalProbabilityFunction*>& cpfs,
                     RewardFunction*& rewardCPF);
    void prepareActions(std::vector<ConditionalProbabilityFunction*> const& cpfs,
                        std::vector<Evaluatable*> const& staticSACs,
                        std::vector<Evaluatable*> const& dynamicSACs,
                        std::vector<ActionFluent*>& actionFluents,
                        std::vector<ActionState>& actionStates);
    void calcPossiblyLegalActionStates(std::vector<ActionFluent*> const& actionFluents,
                                       int actionsToSchedule,
                                       std::list<std::vector<int> >& result,
                                       std::vector<int> addTo = std::vector<int>()) const;
    bool sacContainsNegativeActionFluent(Evaluatable* const& sac, ActionState const& actionState) const;
    bool sacContainsAdditionalPositiveActionFluent(Evaluatable* const& sac, ActionState const& actionState) const;
    void calculateDomains(std::vector<ConditionalProbabilityFunction*>& cpfs,
                          RewardFunction*& rewardCPF,
                          std::vector<ActionState> const& actionStates);
    void finalizeFormulas(std::vector<Evaluatable*>& dynamicSACs,
                          std::vector<Evaluatable*>& stateInvariants,
                          std::vector<ConditionalProbabilityFunction*>& cpfs,
                          RewardFunction*& rewardCPF);
};

#endif
