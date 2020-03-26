#ifndef LOGICAL_EXPRESSIONS_H
#define LOGICAL_EXPRESSIONS_H

#include "states.h"

#include <set>

class NumericConstant;
class StateFluent;
class ConditionalProbabilityFunction;
class LogicalExpression;

typedef std::pair<LogicalExpression*, LogicalExpression*> LogicalExpressionPair;

class LogicalExpression {
public:
    static LogicalExpression* createFromString(std::string& desc);
    static std::vector<LogicalExpression*> createExpressions(std::string& desc);
    static LogicalExpressionPair splitExpressionPair(std::string& desc);

    virtual ~LogicalExpression() {}

    virtual void evaluate(double& res, State const& current,
                          ActionState const& actions) const;
    virtual void evaluateToPD(DiscretePD& res, State const& current,
                              ActionState const& actions) const;
    virtual void evaluateToKleene(std::set<double>& res,
                                  KleeneState const& current,
                                  ActionState const& actions) const;

    virtual void print(std::ostream& out) const = 0;
};

/*****************************************************************
                           Atomics
*****************************************************************/

class StateFluent : public LogicalExpression {
public:
    int index;
    std::string name;
    std::vector<std::string> values;

    void print(std::ostream& out) const override;

protected:
    StateFluent(int _index, std::string _name, std::vector<std::string> _values)
        : index(_index), name(_name), values(_values) {}
};

class DeterministicStateFluent : public StateFluent {
public:
    DeterministicStateFluent(int _index, std::string _name,
                             std::vector<std::string> _values)
        : StateFluent(_index, _name, _values) {}

    void evaluate(double& res, State const& current,
                  ActionState const& actions) const override;
    void evaluateToPD(DiscretePD& res, State const& current,
                      ActionState const& actions) const override;
    void evaluateToKleene(std::set<double>& res, KleeneState const& current,
                          ActionState const& actions) const override;
};

class ProbabilisticStateFluent : public StateFluent {
public:
    ProbabilisticStateFluent(int _index, std::string _name,
                             std::vector<std::string> _values)
        : StateFluent(_index, _name, _values) {}

    void evaluate(double& res, State const& current,
                  ActionState const& actions) const override;
    void evaluateToPD(DiscretePD& res, State const& current,
                      ActionState const& actions) const override;
    void evaluateToKleene(std::set<double>& res, KleeneState const& current,
                          ActionState const& actions) const override;
};

class ActionFluent : public LogicalExpression {
public:
    ActionFluent(int _index, std::string _name,
                 std::vector<std::string> _values)
        : index(_index), name(_name), values(_values) {}

    int index;
    std::string name;
    std::vector<std::string> values;

    void evaluate(double& res, State const& current,
                  ActionState const& actions) const override;
    void evaluateToPD(DiscretePD& res, State const& current,
                      ActionState const& actions) const override;
    void evaluateToKleene(std::set<double>& res, KleeneState const& current,
                          ActionState const& actions) const override;

    void print(std::ostream& out) const override;
};

class NumericConstant : public LogicalExpression {
public:
    NumericConstant(double _value) : value(_value) {}

    double value;

    void evaluate(double& res, State const& current,
                  ActionState const& actions) const override;
    void evaluateToPD(DiscretePD& res, State const& current,
                      ActionState const& actions) const override;
    void evaluateToKleene(std::set<double>& res, KleeneState const& current,
                          ActionState const& actions) const override;

    void print(std::ostream& out) const override;
};

/*****************************************************************
                           Connectives
*****************************************************************/

class Conjunction : public LogicalExpression {
public:
    Conjunction(std::vector<LogicalExpression*>& _exprs) : exprs(_exprs) {}

    std::vector<LogicalExpression*> exprs;

    void evaluate(double& res, State const& current,
                  ActionState const& actions) const override;
    void evaluateToPD(DiscretePD& res, State const& current,
                      ActionState const& actions) const override;
    void evaluateToKleene(std::set<double>& res, KleeneState const& current,
                          ActionState const& actions) const override;

    void print(std::ostream& out) const override;
};

class Disjunction : public LogicalExpression {
public:
    Disjunction(std::vector<LogicalExpression*>& _exprs) : exprs(_exprs) {}

    std::vector<LogicalExpression*> exprs;

    void evaluate(double& res, State const& current,
                  ActionState const& actions) const override;
    void evaluateToPD(DiscretePD& res, State const& current,
                      ActionState const& actions) const override;
    void evaluateToKleene(std::set<double>& res, KleeneState const& current,
                          ActionState const& actions) const override;

    void print(std::ostream& out) const override;
};

class EqualsExpression : public LogicalExpression {
public:
    EqualsExpression(std::vector<LogicalExpression*>& _exprs) : exprs(_exprs) {}

    std::vector<LogicalExpression*> exprs;

    void evaluate(double& res, State const& current,
                  ActionState const& actions) const override;
    void evaluateToPD(DiscretePD& res, State const& current,
                      ActionState const& actions) const override;
    void evaluateToKleene(std::set<double>& res, KleeneState const& current,
                          ActionState const& actions) const override;

    void print(std::ostream& out) const override;
};

class GreaterExpression : public LogicalExpression {
public:
    GreaterExpression(std::vector<LogicalExpression*>& _exprs)
        : exprs(_exprs) {}

    std::vector<LogicalExpression*> exprs;

    void evaluate(double& res, State const& current,
                  ActionState const& actions) const override;
    void evaluateToPD(DiscretePD& res, State const& current,
                      ActionState const& actions) const override;
    void evaluateToKleene(std::set<double>& res, KleeneState const& current,
                          ActionState const& actions) const override;

    void print(std::ostream& out) const override;
};

class LowerExpression : public LogicalExpression {
public:
    LowerExpression(std::vector<LogicalExpression*>& _exprs) : exprs(_exprs) {}

    std::vector<LogicalExpression*> exprs;

    void evaluate(double& res, State const& current,
                  ActionState const& actions) const override;
    void evaluateToPD(DiscretePD& res, State const& current,
                      ActionState const& actions) const override;
    void evaluateToKleene(std::set<double>& res, KleeneState const& current,
                          ActionState const& actions) const override;

    void print(std::ostream& out) const override;
};

class GreaterEqualsExpression : public LogicalExpression {
public:
    GreaterEqualsExpression(std::vector<LogicalExpression*>& _exprs)
        : exprs(_exprs) {}

    std::vector<LogicalExpression*> exprs;

    void evaluate(double& res, State const& current,
                  ActionState const& actions) const override;
    void evaluateToPD(DiscretePD& res, State const& current,
                      ActionState const& actions) const override;
    void evaluateToKleene(std::set<double>& res, KleeneState const& current,
                          ActionState const& actions) const override;

    void print(std::ostream& out) const override;
};

class LowerEqualsExpression : public LogicalExpression {
public:
    LowerEqualsExpression(std::vector<LogicalExpression*>& _exprs)
        : exprs(_exprs) {}

    std::vector<LogicalExpression*> exprs;

    void evaluate(double& res, State const& current,
                  ActionState const& actions) const override;
    void evaluateToPD(DiscretePD& res, State const& current,
                      ActionState const& actions) const override;
    void evaluateToKleene(std::set<double>& res, KleeneState const& current,
                          ActionState const& actions) const override;

    void print(std::ostream& out) const override;
};

class Addition : public LogicalExpression {
public:
    Addition(std::vector<LogicalExpression*>& _exprs) : exprs(_exprs) {}

    std::vector<LogicalExpression*> exprs;

    void evaluate(double& res, State const& current,
                  ActionState const& actions) const override;
    void evaluateToPD(DiscretePD& res, State const& current,
                      ActionState const& actions) const override;
    void evaluateToKleene(std::set<double>& res, KleeneState const& current,
                          ActionState const& actions) const override;

    void print(std::ostream& out) const override;
};

class Subtraction : public LogicalExpression {
public:
    Subtraction(std::vector<LogicalExpression*>& _exprs) : exprs(_exprs) {}

    std::vector<LogicalExpression*> exprs;

    void evaluate(double& res, State const& current,
                  ActionState const& actions) const override;
    void evaluateToPD(DiscretePD& res, State const& current,
                      ActionState const& actions) const override;
    void evaluateToKleene(std::set<double>& res, KleeneState const& current,
                          ActionState const& actions) const override;

    void print(std::ostream& out) const override;
};

class Multiplication : public LogicalExpression {
public:
    Multiplication(std::vector<LogicalExpression*>& _exprs) : exprs(_exprs) {}

    std::vector<LogicalExpression*> exprs;

    void evaluate(double& res, State const& current,
                  ActionState const& actions) const override;
    void evaluateToPD(DiscretePD& res, State const& current,
                      ActionState const& actions) const override;
    void evaluateToKleene(std::set<double>& res, KleeneState const& current,
                          ActionState const& actions) const override;

    void print(std::ostream& out) const override;
};

class Division : public LogicalExpression {
public:
    Division(std::vector<LogicalExpression*>& _exprs) : exprs(_exprs) {}

    std::vector<LogicalExpression*> exprs;

    void evaluate(double& res, State const& current,
                  ActionState const& actions) const override;
    void evaluateToPD(DiscretePD& res, State const& current,
                      ActionState const& actions) const override;
    void evaluateToKleene(std::set<double>& res, KleeneState const& current,
                          ActionState const& actions) const override;

    void print(std::ostream& out) const override;
};

/*****************************************************************
                          Unaries
*****************************************************************/

class Negation : public LogicalExpression {
public:
    Negation(LogicalExpression*& _expr) : expr(_expr) {}

    LogicalExpression* expr;

    void evaluate(double& res, State const& current,
                  ActionState const& actions) const override;
    void evaluateToPD(DiscretePD& res, State const& current,
                      ActionState const& actions) const override;
    void evaluateToKleene(std::set<double>& res, KleeneState const& current,
                          ActionState const& actions) const override;

    void print(std::ostream& out) const override;
};

class ExponentialFunction : public LogicalExpression {
public:
    ExponentialFunction(LogicalExpression*& _expr) : expr(_expr) {}

    LogicalExpression* expr;

    void evaluate(double& res, State const& current,
                  ActionState const& actions) const override;
    void evaluateToPD(DiscretePD& res, State const& current,
                      ActionState const& actions) const override;
    void evaluateToKleene(std::set<double>& res, KleeneState const& current,
                          ActionState const& actions) const override;

    void print(std::ostream& out) const override;
};

/*****************************************************************
                   Probability Distributions
*****************************************************************/

class BernoulliDistribution : public LogicalExpression {
public:
    BernoulliDistribution(LogicalExpression*& _expr) : expr(_expr) {}

    LogicalExpression* expr;

    void evaluate(double& res, State const& current,
                  ActionState const& actions) const override;
    void evaluateToPD(DiscretePD& res, State const& current,
                      ActionState const& actions) const override;
    void evaluateToKleene(std::set<double>& res, KleeneState const& current,
                          ActionState const& actions) const override;

    void print(std::ostream& out) const override;
};

class DiscreteDistribution : public LogicalExpression {
public:
    DiscreteDistribution(std::vector<LogicalExpression*>& _values,
                         std::vector<LogicalExpression*> _probabilities)
        : values(_values), probabilities(_probabilities) {}

    std::vector<LogicalExpression*> values;
    std::vector<LogicalExpression*> probabilities;

    void evaluate(double& res, State const& current,
                  ActionState const& actions) const override;
    void evaluateToPD(DiscretePD& res, State const& current,
                      ActionState const& actions) const override;
    void evaluateToKleene(std::set<double>& res, KleeneState const& current,
                          ActionState const& actions) const override;

    void print(std::ostream& out) const override;
};

/*****************************************************************
                         Conditionals
*****************************************************************/

class MultiConditionChecker : public LogicalExpression {
public:
    MultiConditionChecker(std::vector<LogicalExpression*> _conditions,
                          std::vector<LogicalExpression*> _effects)
        : conditions(_conditions), effects(_effects) {}

    std::vector<LogicalExpression*> conditions;
    std::vector<LogicalExpression*> effects;

    void evaluate(double& res, State const& current,
                  ActionState const& actions) const override;
    void evaluateToPD(DiscretePD& res, State const& current,
                      ActionState const& actions) const override;
    void evaluateToKleene(std::set<double>& res, KleeneState const& current,
                          ActionState const& actions) const override;

    void print(std::ostream& out) const override;
};

#endif
