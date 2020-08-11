#ifndef PRECOMPUTER_H
#define PRECOMPUTER_H

/*
  It can be observed that the (typically prohibitively large) set of states can
  be partitioned into equivalence classes for a given formula which are such
  that all states in the same equivalence class share the same assignments on
  all variables that occur in the formula. If the number of equivalence classes
  is small enough, a representative element of each equivalence class is
  generated and the result of the formula evaluation is precomputed.
*/

#include <vector>

namespace prost::parser {
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
} // namespace prost::parser

#endif
