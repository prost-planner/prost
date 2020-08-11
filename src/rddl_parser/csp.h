#ifndef PARSER_CSP_H
#define PARSER_CSP_H

#include "z3++.h"

#include <vector>

/*
  A RDDLTaskCSP allows to reason over a RDDL task via a CSP. It creates a set
  of z3 state variables for each state variable of the RDDL task, and a set of
  z3 action variables for each action variable of the RDDL task. There are
  different methods to adjust the CSP, e.g. by adding all preconditions as
  constraints, by assigning a set of action variables to specific values or by
  adding arbitrary constraints. Additional sets of state or action variables can
  also be added to the CSP to allow comparison of two (action) states.

  The following example checks if the first two action variables of a given RDDL
  task can be true at the same time:

  RDDLTaskCSP csp(task);
  Z3Expressions const& actionVars = csp.getActionVarSet();
  csp.addPreconditions();
  csp.addConstraint(actionVars[0] == 1 && actionVars[1] == 1);
  bool result = csp.hasSolution();
*/

namespace prost::parser {
using Z3Expressions = std::vector<::z3::expr>;

class LogicalExpression;
struct RDDLTask;

class RDDLTaskCSP {
public:
    explicit RDDLTaskCSP(RDDLTask* _task);

    /*
      Create a set of z3 state variables (i.e., a z3 state variable for each
      state variable of the RDDL task)
    */
    void addStateVarSet();

    /*
      Create a set of z3 action variables (i.e., a z3 action variable for each
      action variable of the RDDL task)
    */
    void addActionVarSet();

    /*
      Constrain the action variable set with index actionSetIndex with the
      preconditions and the concurrency constraint of the RDDL task
    */
    void addPreconditions(int actionSetIndex = 0);

    /*
      Set the action variable set with index actionSetIndex to the given values
    */
    void assignActionVarSet(
        std::vector<int> const& values, int actionSetIndex = 0);

    /*
      Add a constraint to the CSP
    */
    void addConstraint(::z3::expr const& expr) {
        solver.add(expr);
    }

    /*
      Returns the set of z3 state variables with index stateVarSetIndex
    */
    Z3Expressions const& getStateVarSet(int stateVarSetIndex = 0) const {
        assert(stateVarSetIndex < stateVarSets.size());
        return stateVarSets[stateVarSetIndex];
    }

    /*
      Returns the set of z3 action variables with index actionSetIndex
    */
    Z3Expressions const& getActionVarSet(int actionSetIndex = 0) const {
        assert(actionSetIndex < actionVarSets.size());
        return actionVarSets[actionSetIndex];
    }

    /*
      Checks if the current CSP has a solution
    */
    bool hasSolution() {
        return (solver.check() == ::z3::sat);
    }

    /*
      Returns the assignment of all z3 action variables in the action variable
      set with index actionSetIndex
    */
    std::vector<int> getActionModel(int actionSetIndex = 0) const;

    /*
      Adds constraints to the CSP that forbid the current assignment of all z3
      action variables in the action variable set with index actionSetIndex
    */
    void invalidateActionModel(int actionIndex = 0);

    /*
      Remembers the current state of the CSP
    */
    void push() {
        solver.push();
    }

    /*
      Resets the state of the CSP to the state it had when push() was called
      the last time
    */
    void pop() {
        solver.pop();
    }

    /*
      Returns a z3 constant with the given value
    */
    ::z3::expr createConstant(char const* value) {
        return context.real_val(value);
    }

private:
    RDDLTask* task;
    ::z3::context context;
    ::z3::solver solver;
    std::vector<Z3Expressions> stateVarSets;
    std::vector<Z3Expressions> actionVarSets;

    /*
      Adds the concurrency constraint to the CSP (the value given as
      max-nondef-actions is a constraint on the number of concurrently
      applicable action fluents)
    */
    void addConcurrencyConstraint(int actionSetIndex);
};
} // namespace prost::parser

#endif // PARSER_CSP_H
