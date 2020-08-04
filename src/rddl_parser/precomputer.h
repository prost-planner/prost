#ifndef PRECOMPUTER_H
#define PRECOMPUTER_H

#include <vector>

namespace prost {
namespace parser {
class Evaluatable;
struct RDDLTask;
class State;
class StateFluent;


class Precomputer {
public:
    Precomputer(RDDLTask* task)
        : task(task) {}

    void precompute();

private:
    RDDLTask* task;

    void precomputeEvaluatable(Evaluatable* eval);
    void createRelevantStates(std::vector<StateFluent*>& dependentStateFluents,
                              std::vector<State>& result);
    long calculateStateFluentHashKey(Evaluatable* eval,
                                     State const& state) const;
};
} // namespace parser
} // namespace prost

#endif
