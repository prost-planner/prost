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
    Simplifier(RDDLTask *task)
        : task(task), numGeneratedFDRActionFluents(0) {}

    void simplify(bool generateFDRActionFluents, bool output = true);

private:
    RDDLTask *task;
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
    void initializeActionStates();
    void sortActionFluents();

    std::vector<std::set<int>> computeActionFluentMutexes(
        z3::solver& s, std::vector<z3::expr>& af_exprs) const;
};

#endif
