#ifndef CSP_H
#define CSP_H

#include "z3++.h"

#include <vector>

class LogicalExpression;
struct RDDLTask;

using Z3Expressions = std::vector<z3::expr>;

class CSP {
public:
    explicit CSP(RDDLTask* _task);

    void addActionVariables();
    void addPreconditions(int actionIndex = 0);
    void assignActionVariables(
        std::vector<int> const& values, int actionIndex = 0);
    void addConstraint(z3::expr const& expr) {
        solver.add(expr);
    }

    z3::context& getContext() {
        return context;
    }
    Z3Expressions& getState() {
        return state;
    }
    Z3Expressions& getActionVars(int actionIndex = 0) {
        assert(actionIndex < actions.size());
        return actions[actionIndex];
    }

    bool hasSolution() {
        return (solver.check() == z3::sat);
    }

    std::vector<int> getActionModel(int actionIndex = 0) const;
    void invalidateActionModel(int actionIndex = 0);

    void push() {
        solver.push();
    }

    void pop() {
        solver.pop();
    }

    z3::expr getConstant(char const* value) {
        return context.real_val(value);
    }

private:
    RDDLTask* task;
    z3::context context;
    z3::solver solver;
    Z3Expressions state;
    std::vector<Z3Expressions> actions;
};

#endif
