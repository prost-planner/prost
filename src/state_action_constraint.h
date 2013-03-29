#ifndef STATE_ACTION_CONSTRAINT_H
#define STATE_ACTION_CONSTRAINT_H

#include "evaluatable.h"

class UnprocessedPlanningTask;
class RDDLParser;
class Instantiator;

class StateActionConstraint : public Evaluatable {
public:
    static void parse(std::string& desc, UnprocessedPlanningTask* task, RDDLParser* parser);

    void replaceQuantifier(UnprocessedPlanningTask* task, Instantiator* instantiator);
    void instantiate(UnprocessedPlanningTask* task);
    bool simplify(UnprocessedPlanningTask* task, std::map<StateFluent*, NumericConstant*>& replacements);

    bool containsStateFluent() const {
        return !dependentStateFluents.empty();
    }

    bool containsActionFluent() const {
        return !(positiveActionDependencies.empty() && negativeActionDependencies.empty());
    }

    std::set<ActionFluent*> const& getPositiveDependentActionFluents() {
        return positiveActionDependencies;
    }

    std::set<ActionFluent*>const& getNegativeDependentActionFluents() {
        return negativeActionDependencies;
    }

private:
    StateActionConstraint(std::string _name, LogicalExpression* _formula) :
        Evaluatable(_name, _formula) {}
};

#endif
