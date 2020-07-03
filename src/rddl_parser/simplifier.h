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

namespace z3 {
class context;
class expr;
class solver;
}

class Simplifier {
public:
    Simplifier(RDDLTask *task, bool useIPC2018Rules)
        : task(task), useIPC2018Rules(useIPC2018Rules),
          numGeneratedFDRActionFluents(0) {}

    void simplify(bool generateFDRActionFluents, bool output = true);

private:
    RDDLTask *task;
    bool useIPC2018Rules;
    int numGeneratedFDRActionFluents;

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

    bool initializeActionStates();

    void calcAllActionStates(std::vector<ActionState> &result,
                             int minElement, int scheduledActions) const;

    void calcAllActionStatesForIPC2018(std::vector<ActionState> &base,
                                       std::set<ActionState> &result) const;
    void sortActionFluents();

    std::vector<std::set<int>> computeActionFluentMutexes(
        z3::solver& s, std::vector<z3::expr>& af_exprs) const;
};

#endif
