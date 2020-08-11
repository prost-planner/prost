#ifndef DETERMINIZE_H
#define DETERMINIZE_H

/*
  The purpose of a Determinizer is to compute a determinization for all
  (non-deterministic) conditional probability functions. Implement the method

  LogicalExpression* _determinize(LogicalExpression* formula)

  such that it returns the determinization of formula.
*/

namespace prost::parser {
class LogicalExpression;
struct RDDLTask;

namespace determinize {
class Determinizer {
public:
    void determinize();

protected:
    Determinizer(RDDLTask* _task) : task(_task) {}

    virtual LogicalExpression* _determinize(LogicalExpression* formula) = 0;

    RDDLTask* task;
};

/*
  The MostLikelyDeterminizer returns a formula that evaluates to the most likely
  outcome (is several are tied for most likely, the "first" that occurs in
  formula is returned).
*/
class MostLikelyDeterminizer : public Determinizer {
public:
    MostLikelyDeterminizer(RDDLTask* _task) : Determinizer(_task) {}

protected:
    LogicalExpression* _determinize(LogicalExpression* formula) override;
};
} // namespace determinize
} // namespace prost::parser

#endif
