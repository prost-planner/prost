#ifndef SIMPLIFIER_H
#define SIMPLIFIER_H

#include <map>
#include <memory>
#include <set>
#include <vector>

class ActionState;
class ActionPrecondition;
class ConditionalProbabilityFunction;
class FDRGenerator;
class LogicalExpression;
class ParametrizedVariable;
struct RDDLTask;

using Simplifications = std::map<ParametrizedVariable*, LogicalExpression*>;

/*
  The Simplifier class takes a fully instantiated RDDLTask as input (i.e., all
  state and action fluents, CPFs, action preconditions and the reward function
  are grounded naively) and simplifies the input in a fixed point iteration that
  performs the following steps until nothing changes anymore:

  1. Simplify all CPFs, preconditions and the reward function by replacing all
     state and action fluents that are shown to simplify to their initial value
     in any of the methods the fixed point iteration with their initial value.
  2. Check if a precondition forbids the application of an action fluent
     trivially, i.e. if it is of the form "not a" for some action fluent a.
  3. Determine which action fluents are relevant, i.e. if they occur at least
     once in a CPF, precondition or the reward function.
  4. Generate finite domain action fluents by determining mutually exclusive
     (mutex) action fluents and combining them into one FDR action fluent (there
     is a parameter that allows to skip this step).
  5. Compute all actions for which there is at least one state where all action
     preconditions are satisfied.
  6. Perform a (simple) reachability analysis.

  In a final step (outside the fixed point iteration), relevant preconditions
  for each action are determined, and preconditions that are irrelevant for all
  actions are removed.

  The output of the Simplifier class is an instantiated RDDLTask where (some)
  constant state fluents and their associated CPFs, action fluents and
  irrelevant preconditions have been removed.
*/

class Simplifier {
public:
    Simplifier()  = delete;
    explicit Simplifier(RDDLTask* _task);

    void simplify(bool generateFDRActionFluents, bool output = true);

private:
    RDDLTask* task;
    // TODO: This should be a unique_ptr, which requires c++14
    std::shared_ptr<FDRGenerator> fdrGen;
    int numGeneratedFDRActionFluents;

    /*
      Simplify all CPFs, preconditions and the reward function by replacing all
      state and action fluents that are shown to simplify to their initial
      value with their initial value
    */
    void simplifyFormulas(Simplifications& replacements);

    /*
      Apply the given replacements to all CPFs until no CPF simplifies to
      its initial value
    */
    void simplifyCPFs(Simplifications& replacements);

    /*
      Apply the given replacements to all preconditions and split the result
      into separate conjunctive elements if possible
    */
    void simplifyPreconditions(Simplifications& replacements);
    std::vector<ActionPrecondition*> simplifyPrecondition(
        ActionPrecondition* precond, Simplifications& replacements);

    /*
      Check if a precondition forbids the application of an action fluent
      trivially, i.e. if it is of the form "not a" for some action fluent a
    */
    bool computeInapplicableActionFluents(Simplifications& replacements);

    /*
      Determine which action fluents are relevant, i.e. those that occur in some
      precondition, CPF or the reward formula
    */
    bool computeRelevantActionFluents(Simplifications& replacements);

    /*
      Generate finite domain action fluents by determining mutually exclusive
      (mutex) action fluents and combining them into one FDR action fluent
    */
    bool determineFiniteDomainActionFluents(Simplifications& replacements);

    /*
      Compute all actions that are applicable in at least one state and
      determine which action variables are used in these actions
    */
    bool computeActions(Simplifications& replacements);

    /*
      Compute all actions that are applicable in at least one state
    */
    std::vector<ActionState> computeApplicableActions() const;

    /*
      Perform a (simple) reachability analysis
    */
    bool approximateDomains(Simplifications& replacements);

    /*
      Determines which state fluents are constant based on the domains, adds
      those to the replacements and returns the non-constant ones.
    */
    bool removeConstantStateFluents(std::vector<std::set<double>> const& domains,
                                    Simplifications& replacements);

    /*
      Determine which preconditions are relevant during search and associate
      these preconditions with the actions they can possibly influence
    */
    void determineRelevantPreconditions();

    /*
      Remove all action fluents af where filter[af->index] is false, assign new
      indices to those that are kept, update replacements and return true if at
      least one action fluent was filtered
    */
    bool filterActionFluents(
        std::vector<bool> const& filter, Simplifications& replacements);
};

#endif
