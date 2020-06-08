#ifndef SIMPLIFIER_H
#define SIMPLIFIER_H

#include <map>
#include <set>
#include <vector>

class ActionState;
class ActionPrecondition;
class LogicalExpression;
class ParametrizedVariable;
class RDDLTask;

class Simplifier {
public:
    Simplifier(RDDLTask *task, bool useIPC2018Rules)
        : task(task), useIPC2018Rules(useIPC2018Rules) {}

    void simplify(bool generateFDRActionFluents, bool output = true);

private:
    RDDLTask *task;
    bool useIPC2018Rules;

    void simplifyFormulas(
        std::map<ParametrizedVariable *, LogicalExpression *> &replacements);

    bool computeInapplicableActionFluents(
        std::map<ParametrizedVariable *, LogicalExpression *> &replacements);

    bool computeRelevantActionFluents(
        std::map<ParametrizedVariable *, LogicalExpression *> &replacements);

    bool determineFiniteDomainActionFluents(
        std::map<ParametrizedVariable*, LogicalExpression*>& replacements);

    bool computeActions(
        std::map<ParametrizedVariable *, LogicalExpression *> &replacements);

    bool approximateDomains(
        std::map<ParametrizedVariable *, LogicalExpression *> &replacements);

    bool actionIsApplicable(ActionState const &action) const;

    void initializeActionStates();

    void calcAllActionStates(std::vector<ActionState> &result,
                             int minElement, int scheduledActions) const;

    void calcAllActionStatesForIPC2018(std::vector<ActionState> &base,
                                       std::set<ActionState> &result) const;

    bool sacContainsNegativeActionFluent(ActionPrecondition *const &sac,
                                         ActionState const &actionState) const;

    bool sacContainsAdditionalPositiveActionFluent(
        ActionPrecondition *const &sac, ActionState const &actionState) const;
};

#endif
