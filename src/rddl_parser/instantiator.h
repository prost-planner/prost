#ifndef INSTANTIATOR_H
#define INSTANTIATOR_H

/*
  The Instantiator class receives the lifted planning task as modelled in RDDL
  as input and naively grounds (i.e., instantiates all parameters with concrete
  objects) all state and action variables, CPFs action preconditions. In the
  process, all quantifiers that occur in formulas are replaced by corresponding
  expressions over the set of objets.
*/

#include <vector>

namespace prost::parser {
class Parameter;
class ParametrizedVariable;
class LogicalExpression;
struct RDDLTask;

class Instantiator {
public:
    Instantiator(RDDLTask* _task) : task(_task) {}

    void instantiate(bool const& output = true);
    void instantiateParams(
        std::vector<Parameter*> params,
        std::vector<std::vector<Parameter*>>& result,
        std::vector<Parameter*> addTo = std::vector<Parameter*>(),
        int indexToProcess = 0);

private:
    RDDLTask* task;

    void instantiateVariables();
    void instantiateCPFs();
    void instantiateCPF(ParametrizedVariable* head, LogicalExpression* formula);
    void instantiatePreconds();
};
} // namespace prost::parser

#endif
