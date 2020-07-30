#ifndef REACHABILITY_ANALYSIS_H
#define REACHABILITY_ANALYSIS_H

#include <set>
#include <vector>

class ConditionalProbabilityFunction;
struct RDDLTask;

/*
  A ReachabilityAnalyser provides a method to compute (exactly or as an
  overapproximation) the set of reachable values for each state variable
*/
class ReachabilityAnalyser {
public:
    virtual std::vector<std::set<double>> determineReachableFacts() = 0;

protected:
    ReachabilityAnalyser(RDDLTask* _task)
        : task(_task) {}

    RDDLTask* task;
};

/*
  The MinkowskiReachabilityAnalyser overapproximates reachable facts by
  calculating successors based on arithmetic that is inspired by the Minkowski
  sum: the result of some (binary, but can equivalently be defined for unary or
  n-ary operators) operator o applied to two sets s1 and s2 is the set
  {x o y | x \in s1 and y \in s2}.
*/
class MinkowskiReachabilityAnalyser : public ReachabilityAnalyser {
public:
    MinkowskiReachabilityAnalyser(RDDLTask* _task);

    std::vector<std::set<double>> determineReachableFacts() override;

    /*
      Returns the step counter in which the fixed point was reached (i.e., the
      step where nothing changes anymore - the last step that added some value
      is this value minus 1).
    */
    int getNumberOfSimulationSteps() const {
        return step;
    }

private:
    int step;
    std::vector<std::vector<int>> actionIndicesByCPF;

    /*
      To apply as few actions as possible, the set of action states is
      partitioned into equivalence classes for each CPF, and just one
      representative action from each equivalence class is used. Two actions are
      in the same equivalence class if the action variable assignment of all
      action variable that are relevant for the CPF are equal.
    */
    void prepareActionEquivalence();

    /*
      Performs one step, i.e. evaluates each CPF by applying all representative
      actions and extends the domain of the corresponding state variable.
    */
    std::vector<std::set<double>> performStep(
        std::vector<std::set<double>>& values) const;

    /*
      Applies all representative actions to the given CPF and returns all
      reachable values.
    */
    std::set<double> applyRepresentativeActions(
        ConditionalProbabilityFunction* cpf,
        std::vector<std::set<double>> const& values) const;
};

#endif
