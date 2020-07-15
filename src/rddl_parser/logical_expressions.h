#ifndef LOGICAL_EXPRESSIONS_H
#define LOGICAL_EXPRESSIONS_H

#include <map>
#include <set>
#include <string>
#include <vector>

class ActionFluent;
class ActionState;
class CSP;
class DiscretePD;
class Instantiator;
class KleeneState;
class LogicalExpression;
class NumericConstant;
class Object;
class ParametrizedVariable;
struct RDDLTask;
class State;
class StateFluent;

namespace z3 {
class expr;
}

using ActionFluentSet = std::set<ActionFluent*>;
using Domains = std::vector<std::set<double>>;
using Instantiations = std::map<std::string, Object*>;
using Simplifications = std::map<ParametrizedVariable*, LogicalExpression*>;
using StateFluentSet = std::set<StateFluent*>;
using Z3Expressions = std::vector<z3::expr>;

class LogicalExpression {
public:
    virtual ~LogicalExpression() {}

    virtual LogicalExpression* replaceQuantifier(Instantiations& replace,
                                                 Instantiator* inst);
    virtual LogicalExpression* instantiate(RDDLTask* task,
                                           Instantiations& replace);
    virtual LogicalExpression* simplify(Simplifications& replace);

    virtual LogicalExpression* determinizeMostLikely(
        std::vector<ActionState> const& actionStates);

    virtual void collectInitialInfo(bool& isProbabilistic,
                                    bool& containsArithmeticFunction,
                                    StateFluentSet& dependentStateFluents,
                                    ActionFluentSet& dependentActionFluents);
    virtual void calculateDomain(Domains const& domains,
                                 ActionState const& action,
                                 std::set<double>& res);
    virtual void calculateDomainAsInterval(Domains const& domains,
                                           ActionState const& action,
                                           double& minRes, double& maxRes);

    virtual void evaluate(double& res, State const& current,
                          ActionState const& action) const;
    virtual void evaluateToPD(DiscretePD& res, State const& current,
                              ActionState const& action) const;
    virtual void evaluateToKleene(std::set<double>& res,
                                  KleeneState const& current,
                                  ActionState const& action) const;

    void determineBounds(std::vector<ActionState> const& actionStates,
                         double& minRes, double& maxRes) const;
    virtual void determineBounds(ActionState const& action, double& minRes,
                                 double& maxRes) const;

    virtual z3::expr toZ3Formula(CSP& csp, int actionIndex) const;

    virtual void print(std::ostream& out) const;
};

/*****************************************************************
                         Schematics
*****************************************************************/

class Type {
public:
    Type(std::string _name, Type* _superType = nullptr)
        : name(_name), superType(_superType) {}

    std::string name;
    Type* superType;
    std::vector<Object*> objects;

    bool isSubtypeOf(Type* const& other) const;
};

// We need Parameter and Object as LogicalExpressions because of

// 1. It simplifies parsing of static parameters, but that does not mean they
// are necessary.

// 2. (In-)Equality checks of the form (?p1 == ?p2), (?p == object-name) or (?p
// ~= @enum-val). These are replaced in instantiate.

// 3. Dynamic parameters like var( var2( type2 ) ) where var2( type2 ) is an
// object or enum fluent that evaluates to type1, the parameter type of var(
// type1 ). TODO: These are currently not supported, change that!

class Parameter : public LogicalExpression {
public:
    Parameter(std::string _name, Type* _type = nullptr)
        : name(_name), type(_type) {}

    std::string name;
    Type* type;

    LogicalExpression* replaceQuantifier(
        Instantiations& replace, Instantiator* inst)  override;
    LogicalExpression* instantiate(
        RDDLTask* task, Instantiations& replace) override;

    void print(std::ostream& out) const override;
};

class Object : public Parameter {
public:
    Object(std::string _name, Type* _type);
    Object(std::string _name, Type* _type, double _value)
        : Parameter(_name, _type), types({_type}), value(_value) {}
    ~Object() {}

    std::vector<Type*> types;
    double value;

    LogicalExpression* replaceQuantifier(
        Instantiations& replace, Instantiator* inst) override;
    LogicalExpression* instantiate(
        RDDLTask* task, Instantiations& replace) override;

    void print(std::ostream& out) const override;
};

class ParametrizedVariable : public LogicalExpression {
public:
    enum VariableType {
        STATE_FLUENT,
        ACTION_FLUENT,
        // DERIVED_FLUENT,
        // INTERM_FLUENT,
        NON_FLUENT
    };

    ParametrizedVariable(std::string _variableName,
                         std::vector<Parameter*> _params,
                         VariableType _variableType, Type* _valueType,
                         double _initialValue)
        : LogicalExpression(),
          variableName(_variableName),
          fullName(_variableName),
          params(_params),
          variableType(_variableType),
          valueType(_valueType),
          initialValue(_initialValue) {}
    ParametrizedVariable(ParametrizedVariable const& source,
                         std::vector<Parameter*> _params);
    ParametrizedVariable(ParametrizedVariable const& source,
                         std::vector<Parameter*> _params, double _initialValue);

    std::string variableName;
    std::string fullName;
    std::vector<Parameter*> params;
    VariableType variableType;
    Type* valueType;
    double initialValue;

    LogicalExpression* replaceQuantifier(
        Instantiations& replace, Instantiator* inst) override;
    LogicalExpression* instantiate(
        RDDLTask* task, Instantiations& replace) override;
    LogicalExpression* simplify(Simplifications& replace) override;
    LogicalExpression* determinizeMostLikely(
        std::vector<ActionState> const& actionStates) override;

    void print(std::ostream& out) const override;
};

/*****************************************************************
                           Atomics
*****************************************************************/

class StateFluent : public ParametrizedVariable {
public:
    StateFluent(ParametrizedVariable const& source,
                std::vector<Parameter*> _params, double _initialValue)
        : ParametrizedVariable(source, _params, _initialValue), index(-1) {}
    StateFluent(ParametrizedVariable const& source,
                std::vector<Parameter*> _params)
        : ParametrizedVariable(source, _params), index(-1) {}

    int index;
    int domainSize;

    LogicalExpression* simplify(Simplifications& replace) override;

    void collectInitialInfo(bool& isProbabilistic,
                            bool& containsArithmeticFunction,
                            StateFluentSet& dependentStateFluents,
                            ActionFluentSet& dependentActionFluents) override;
    void calculateDomain(Domains const& domains, ActionState const& action,
                         std::set<double>& res) override;
    void calculateDomainAsInterval(Domains const& domains,
                                   ActionState const& action, double& minRes,
                                   double& maxRes) override;

    void evaluate(double& res, State const& current,
                  ActionState const& action) const override;
    void evaluateToPD(DiscretePD& res, State const& current,
                      ActionState const& action) const override;
    void evaluateToKleene(std::set<double>& res, KleeneState const& current,
                          ActionState const& action) const override;

    void determineBounds(ActionState const& action, double& minRes,
                         double& maxRes) const override;

    z3::expr toZ3Formula(CSP& csp, int actionIndex) const override;

    void print(std::ostream& out) const override;
};

class ActionFluent : public ParametrizedVariable {
public:
    ActionFluent(ParametrizedVariable const& source,
                 std::vector<Parameter*> _params)
        : ParametrizedVariable(source, _params, 0.0), index(-1) {}
    ActionFluent(std::string name, Type* _valueType)
        : ParametrizedVariable(name, std::vector<Parameter*>(), VariableType::ACTION_FLUENT, _valueType, 0.0),
          index(-1) {}

    int index;

    LogicalExpression* simplify(Simplifications& replace) override;

    void collectInitialInfo(bool& isProbabilistic,
                            bool& containsArithmeticFunction,
                            StateFluentSet& dependentStateFluents,
                            ActionFluentSet& dependentActionFluents) override;
    void calculateDomain(Domains const& domains, ActionState const& action,
                         std::set<double>& res) override;
    void calculateDomainAsInterval(Domains const& domains,
                                   ActionState const& action, double& minRes,
                                   double& maxRes) override;

    void evaluate(double& res, State const& current,
                  ActionState const& action) const override;
    void evaluateToPD(DiscretePD& res, State const& current,
                      ActionState const& action) const override;
    void evaluateToKleene(std::set<double>& res, KleeneState const& current,
                          ActionState const& action) const override;

    void determineBounds(ActionState const& action, double& minRes,
                         double& maxRes) const override;

    z3::expr toZ3Formula(CSP& csp, int actionIndex) const override;

    void print(std::ostream& out) const override;
};

class NonFluent : public ParametrizedVariable {
public:
    NonFluent(ParametrizedVariable const& source,
              std::vector<Parameter*> _params, double _initialValue)
        : ParametrizedVariable(source, _params, _initialValue) {}
    NonFluent(ParametrizedVariable const& source,
              std::vector<Parameter*> _params)
        : ParametrizedVariable(source, _params) {}
};

class NumericConstant : public LogicalExpression {
public:
    NumericConstant(double _value) : value(_value) {}

    double value;

    LogicalExpression* replaceQuantifier(
        Instantiations& replace, Instantiator* inst) override;
    LogicalExpression* instantiate(
        RDDLTask* task, Instantiations& replace) override;
    LogicalExpression* simplify(Simplifications& replace) override;

    LogicalExpression* determinizeMostLikely(
        std::vector<ActionState> const& actionStates) override;

    void collectInitialInfo(bool& isProbabilistic,
                            bool& containsArithmeticFunction,
                            StateFluentSet& dependentStateFluents,
                            ActionFluentSet& dependentActionFluents) override;
    void calculateDomain(Domains const& domains, ActionState const& action,
                         std::set<double>& res) override;

    void calculateDomainAsInterval(Domains const& domains,
                                   ActionState const& action, double& minRes,
                                   double& maxRes) override;

    void evaluate(double& res, State const& current,
                  ActionState const& action) const override;
    void evaluateToPD(DiscretePD& res, State const& current,
                      ActionState const& action) const override;
    void evaluateToKleene(std::set<double>& res, KleeneState const& current,
                          ActionState const& action) const override;

    void determineBounds(ActionState const& action, double& minRes,
                         double& maxRes) const override;

    z3::expr toZ3Formula(CSP& csp, int actionIndex) const override;

    void print(std::ostream& out) const override;
};

/*****************************************************************
                          Quantifier
*****************************************************************/

class ParameterList : public LogicalExpression {
public:
    ParameterList(std::vector<Parameter*> _params, std::vector<Type*> _types)
        : params(_params), types(_types) {}
    ParameterList() {}

    std::vector<Parameter*> params;
    std::vector<Type*> types;

    void print(std::ostream& out) const override;
};

class Quantifier : public LogicalExpression {
public:
    Quantifier(ParameterList* _paramList, LogicalExpression* _expr)
        : paramList(_paramList), expr(_expr) {}

    ParameterList* paramList;
    LogicalExpression* expr;

    void getReplacements(std::vector<std::string>& parameterNames,
                         std::vector<std::vector<Parameter*>>& replace,
                         Instantiator* inst);
};

class Sumation : public Quantifier {
public:
    Sumation(ParameterList* _paramList, LogicalExpression* _expr)
        : Quantifier(_paramList, _expr) {}

    LogicalExpression* replaceQuantifier(
        Instantiations& replace, Instantiator* inst) override;

    void print(std::ostream& out) const override;
};

class Product : public Quantifier {
public:
    Product(ParameterList* _paramList, LogicalExpression* _expr)
        : Quantifier(_paramList, _expr) {}

    LogicalExpression* replaceQuantifier(
        Instantiations& replace, Instantiator* inst) override;

    void print(std::ostream& out) const override;
};

class UniversalQuantification : public Quantifier {
public:
    UniversalQuantification(ParameterList* _paramList, LogicalExpression* _expr)
        : Quantifier(_paramList, _expr) {}

    LogicalExpression* replaceQuantifier(
        Instantiations& replace, Instantiator* inst) override;

    void print(std::ostream& out) const override;
};

class ExistentialQuantification : public Quantifier {
public:
    ExistentialQuantification(ParameterList* _paramList,
                              LogicalExpression* _expr)
        : Quantifier(_paramList, _expr) {}

    LogicalExpression* replaceQuantifier(
        Instantiations& replace, Instantiator* inst) override;

    void print(std::ostream& out) const override;
};

/*****************************************************************
                           Connectives
*****************************************************************/

class Connective : public LogicalExpression {
public:
    std::vector<LogicalExpression*> exprs;

    Connective(std::vector<LogicalExpression*>& _exprs) : exprs(_exprs) {}

    void print(std::ostream& out) const override;
};

class Conjunction : public Connective {
public:
    Conjunction(std::vector<LogicalExpression*>& _exprs) : Connective(_exprs) {}

    LogicalExpression* replaceQuantifier(
        Instantiations& replace, Instantiator* inst) override;
    LogicalExpression* instantiate(
        RDDLTask* task, Instantiations& replace) override;
    LogicalExpression* simplify(Simplifications& replace) override;

    LogicalExpression* determinizeMostLikely(
        std::vector<ActionState> const& actionStates) override;

    void collectInitialInfo(bool& isProbabilistic,
                            bool& containsArithmeticFunction,
                            StateFluentSet& dependentStateFluents,
                            ActionFluentSet& dependentActionFluents) override;
    void calculateDomain(Domains const& domains, ActionState const& action,
                         std::set<double>& res) override;
    void calculateDomainAsInterval(Domains const& domains,
                                   ActionState const& action, double& minRes,
                                   double& maxRes) override;

    void evaluate(double& res, State const& current,
                  ActionState const& action) const override;
    void evaluateToPD(DiscretePD& res, State const& current,
                      ActionState const& action) const override;
    void evaluateToKleene(std::set<double>& res, KleeneState const& current,
                          ActionState const& action) const override;

    void determineBounds(ActionState const& action, double& minRes,
                         double& maxRes) const override;

    z3::expr toZ3Formula(CSP& csp, int actionIndex) const override;

    void print(std::ostream& out) const override;
};

class Disjunction : public Connective {
public:
    Disjunction(std::vector<LogicalExpression*>& _exprs) : Connective(_exprs) {}

    LogicalExpression* replaceQuantifier(
        Instantiations& replace, Instantiator* inst) override;
    LogicalExpression* instantiate(
        RDDLTask* task, Instantiations& replace) override;
    LogicalExpression* simplify(Simplifications& replace) override;

    LogicalExpression* determinizeMostLikely(
        std::vector<ActionState> const& actionStates) override;

    void collectInitialInfo(bool& isProbabilistic,
                            bool& containsArithmeticFunction,
                            StateFluentSet& dependentStateFluents,
                            ActionFluentSet& dependentActionFluents) override;
    void calculateDomain(Domains const& domains, ActionState const& action,
                         std::set<double>& res) override;
    void calculateDomainAsInterval(Domains const& domains,
                                   ActionState const& action, double& minRes,
                                   double& maxRes) override;

    void evaluate(double& res, State const& current,
                  ActionState const& action) const override;
    void evaluateToPD(DiscretePD& res, State const& current,
                      ActionState const& action) const override;
    void evaluateToKleene(std::set<double>& res, KleeneState const& current,
                          ActionState const& action) const override;

    void determineBounds(ActionState const& action, double& minRes,
                         double& maxRes) const override;

    z3::expr toZ3Formula(CSP& csp, int actionIndex) const override;

    void print(std::ostream& out) const override;
};

class EqualsExpression : public Connective {
public:
    EqualsExpression(std::vector<LogicalExpression*>& _exprs)
        : Connective(_exprs) {}

    LogicalExpression* replaceQuantifier(
        Instantiations& replace, Instantiator* inst) override;
    LogicalExpression* instantiate(
        RDDLTask* task, Instantiations& replace) override;
    LogicalExpression* simplify(Simplifications& replace) override;

    LogicalExpression* determinizeMostLikely(
        std::vector<ActionState> const& actionStates) override;

    void collectInitialInfo(bool& isProbabilistic,
                            bool& containsArithmeticFunction,
                            StateFluentSet& dependentStateFluents,
                            ActionFluentSet& dependentActionFluents) override;
    void calculateDomain(Domains const& domains, ActionState const& action,
                         std::set<double>& res) override;
    void calculateDomainAsInterval(Domains const& domains,
                                   ActionState const& action, double& minRes,
                                   double& maxRes) override;

    void evaluate(double& res, State const& current,
                  ActionState const& action) const override;
    void evaluateToPD(DiscretePD& res, State const& current,
                      ActionState const& action) const override;
    void evaluateToKleene(std::set<double>& res, KleeneState const& current,
                          ActionState const& action) const override;

    void determineBounds(ActionState const& action, double& minRes,
                         double& maxRes) const override;

    z3::expr toZ3Formula(CSP& csp, int actionIndex) const override;

    void print(std::ostream& out) const override;
};

class GreaterExpression : public Connective {
public:
    GreaterExpression(std::vector<LogicalExpression*>& _exprs)
        : Connective(_exprs) {}

    LogicalExpression* replaceQuantifier(
        Instantiations& replace, Instantiator* inst) override;
    LogicalExpression* instantiate(
        RDDLTask* task, Instantiations& replace) override;
    LogicalExpression* simplify(Simplifications& replace) override;

    LogicalExpression* determinizeMostLikely(
        std::vector<ActionState> const& actionStates) override;

    void collectInitialInfo(bool& isProbabilistic,
                            bool& containsArithmeticFunction,
                            StateFluentSet& dependentStateFluents,
                            ActionFluentSet& dependentActionFluents) override;
    void calculateDomain(Domains const& domains, ActionState const& action,
                         std::set<double>& res) override;
    void calculateDomainAsInterval(Domains const& domains,
                                   ActionState const& action, double& minRes,
                                   double& maxRes) override;

    void evaluate(double& res, State const& current,
                  ActionState const& action) const override;
    void evaluateToPD(DiscretePD& res, State const& current,
                      ActionState const& action) const override;
    void evaluateToKleene(std::set<double>& res, KleeneState const& current,
                          ActionState const& action) const override;

    void determineBounds(ActionState const& action, double& minRes,
                         double& maxRes) const override;

    z3::expr toZ3Formula(CSP& csp, int actionIndex) const override;

    void print(std::ostream& out) const override;
};

class LowerExpression : public Connective {
public:
    LowerExpression(std::vector<LogicalExpression*>& _exprs)
        : Connective(_exprs) {}

    LogicalExpression* replaceQuantifier(
        Instantiations& replace, Instantiator* inst) override;
    LogicalExpression* instantiate(
        RDDLTask* task, Instantiations& replace) override;
    LogicalExpression* simplify(Simplifications& replace) override;

    LogicalExpression* determinizeMostLikely(
        std::vector<ActionState> const& actionStates) override;

    void collectInitialInfo(bool& isProbabilistic,
                            bool& containsArithmeticFunction,
                            StateFluentSet& dependentStateFluents,
                            ActionFluentSet& dependentActionFluents) override;
    void calculateDomain(Domains const& domains, ActionState const& action,
                         std::set<double>& res) override;
    void calculateDomainAsInterval(Domains const& domains,
                                   ActionState const& action, double& minRes,
                                   double& maxRes) override;

    void evaluate(double& res, State const& current,
                  ActionState const& action) const override;
    void evaluateToPD(DiscretePD& res, State const& current,
                      ActionState const& action) const override;
    void evaluateToKleene(std::set<double>& res, KleeneState const& current,
                          ActionState const& action) const override;

    void determineBounds(ActionState const& action, double& minRes,
                         double& maxRes) const override;

    z3::expr toZ3Formula(CSP& csp, int actionIndex) const override;

    void print(std::ostream& out) const override;
};

class GreaterEqualsExpression : public Connective {
public:
    GreaterEqualsExpression(std::vector<LogicalExpression*>& _exprs)
        : Connective(_exprs) {}

    LogicalExpression* replaceQuantifier(
        Instantiations& replace, Instantiator* inst) override;
    LogicalExpression* instantiate(
        RDDLTask* task, Instantiations& replace) override;
    LogicalExpression* simplify(Simplifications& replace) override;

    LogicalExpression* determinizeMostLikely(
        std::vector<ActionState> const& actionStates) override;

    void collectInitialInfo(bool& isProbabilistic,
                            bool& containsArithmeticFunction,
                            StateFluentSet& dependentStateFluents,
                            ActionFluentSet& dependentActionFluents) override;
    void calculateDomain(Domains const& domains, ActionState const& action,
                         std::set<double>& res) override;
    void calculateDomainAsInterval(Domains const& domains,
                                   ActionState const& action, double& minRes,
                                   double& maxRes) override;

    void evaluate(double& res, State const& current,
                  ActionState const& action) const override;
    void evaluateToPD(DiscretePD& res, State const& current,
                      ActionState const& action) const override;
    void evaluateToKleene(std::set<double>& res, KleeneState const& current,
                          ActionState const& action) const override;

    void determineBounds(ActionState const& action, double& minRes,
                         double& maxRes) const override;

    z3::expr toZ3Formula(CSP& csp, int actionIndex) const override;

    void print(std::ostream& out) const override;
};

class LowerEqualsExpression : public Connective {
public:
    LowerEqualsExpression(std::vector<LogicalExpression*>& _exprs)
        : Connective(_exprs) {}

    LogicalExpression* replaceQuantifier(
        Instantiations& replace, Instantiator* inst) override;
    LogicalExpression* instantiate(
        RDDLTask* task, Instantiations& replace) override;
    LogicalExpression* simplify(Simplifications& replace) override;

    LogicalExpression* determinizeMostLikely(
        std::vector<ActionState> const& actionStates) override;

    void collectInitialInfo(bool& isProbabilistic,
                            bool& containsArithmeticFunction,
                            StateFluentSet& dependentStateFluents,
                            ActionFluentSet& dependentActionFluents) override;
    void calculateDomain(Domains const& domains, ActionState const& action,
                         std::set<double>& res) override;
    void calculateDomainAsInterval(Domains const& domains,
                                   ActionState const& action, double& minRes,
                                   double& maxRes) override;

    void evaluate(double& res, State const& current,
                  ActionState const& action) const override;
    void evaluateToPD(DiscretePD& res, State const& current,
                      ActionState const& action) const override;
    void evaluateToKleene(std::set<double>& res, KleeneState const& current,
                          ActionState const& action) const override;

    void determineBounds(ActionState const& action, double& minRes,
                         double& maxRes) const override;

    z3::expr toZ3Formula(CSP& csp, int actionIndex) const override;

    void print(std::ostream& out) const override;
};

class Addition : public Connective {
public:
    Addition(std::vector<LogicalExpression*>& _exprs) : Connective(_exprs) {}

    LogicalExpression* replaceQuantifier(
        Instantiations& replace, Instantiator* inst) override;
    LogicalExpression* instantiate(
        RDDLTask* task, Instantiations& replace) override;
    LogicalExpression* simplify(Simplifications& replace) override;

    LogicalExpression* determinizeMostLikely(
        std::vector<ActionState> const& actionStates) override;

    void collectInitialInfo(bool& isProbabilistic,
                            bool& containsArithmeticFunction,
                            StateFluentSet& dependentStateFluents,
                            ActionFluentSet& dependentActionFluents) override;
    void calculateDomain(Domains const& domains, ActionState const& action,
                         std::set<double>& res) override;
    void calculateDomainAsInterval(Domains const& domains,
                                   ActionState const& action, double& minRes,
                                   double& maxRes) override;

    void evaluate(double& res, State const& current,
                  ActionState const& action) const override;
    void evaluateToPD(DiscretePD& res, State const& current,
                      ActionState const& action) const override;
    void evaluateToKleene(std::set<double>& res, KleeneState const& current,
                          ActionState const& action) const override;

    void determineBounds(ActionState const& action, double& minRes,
                         double& maxRes) const override;

    z3::expr toZ3Formula(CSP& csp, int actionIndex) const override;

    void print(std::ostream& out) const override;
};

class Subtraction : public Connective {
public:
    Subtraction(std::vector<LogicalExpression*>& _exprs) : Connective(_exprs) {}

    LogicalExpression* replaceQuantifier(
        Instantiations& replace, Instantiator* inst) override;
    LogicalExpression* instantiate(
        RDDLTask* task, Instantiations& replace) override;
    LogicalExpression* simplify(Simplifications& replace) override;

    LogicalExpression* determinizeMostLikely(
        std::vector<ActionState> const& actionStates) override;

    void collectInitialInfo(bool& isProbabilistic,
                            bool& containsArithmeticFunction,
                            StateFluentSet& dependentStateFluents,
                            ActionFluentSet& dependentActionFluents) override;
    void calculateDomain(Domains const& domains, ActionState const& action,
                         std::set<double>& res) override;
    void calculateDomainAsInterval(Domains const& domains,
                                   ActionState const& action, double& minRes,
                                   double& maxRes) override;

    void evaluate(double& res, State const& current,
                  ActionState const& action) const override;
    void evaluateToPD(DiscretePD& res, State const& current,
                      ActionState const& action) const override;
    void evaluateToKleene(std::set<double>& res, KleeneState const& current,
                          ActionState const& action) const override;

    void determineBounds(ActionState const& action, double& minRes,
                         double& maxRes) const override;

    z3::expr toZ3Formula(CSP& csp, int actionIndex) const override;

    void print(std::ostream& out) const override;
};

class Multiplication : public Connective {
public:
    Multiplication(std::vector<LogicalExpression*>& _exprs)
        : Connective(_exprs) {}

    LogicalExpression* replaceQuantifier(
        Instantiations& replace, Instantiator* inst) override;
    LogicalExpression* instantiate(
        RDDLTask* task, Instantiations& replace) override;
    LogicalExpression* simplify(Simplifications& replace) override;

    LogicalExpression* determinizeMostLikely(
        std::vector<ActionState> const& actionStates) override;

    void collectInitialInfo(bool& isProbabilistic,
                            bool& containsArithmeticFunction,
                            StateFluentSet& dependentStateFluents,
                            ActionFluentSet& dependentActionFluents) override;
    void calculateDomain(Domains const& domains, ActionState const& action,
                         std::set<double>& res) override;
    void calculateDomainAsInterval(Domains const& domains,
                                   ActionState const& action, double& minRes,
                                   double& maxRes) override;

    void evaluate(double& res, State const& current,
                  ActionState const& action) const override;
    void evaluateToPD(DiscretePD& res, State const& current,
                      ActionState const& action) const override;
    void evaluateToKleene(std::set<double>& res, KleeneState const& current,
                          ActionState const& action) const override;

    void determineBounds(ActionState const& action, double& minRes,
                         double& maxRes) const override;

    z3::expr toZ3Formula(CSP& csp, int actionIndex) const override;

    void print(std::ostream& out) const override;
};

class Division : public Connective {
public:
    Division(std::vector<LogicalExpression*>& _exprs) : Connective(_exprs) {}

    LogicalExpression* replaceQuantifier(
        Instantiations& replace, Instantiator* inst) override;
    LogicalExpression* instantiate(
        RDDLTask* task, Instantiations& replace) override;
    LogicalExpression* simplify(Simplifications& replace) override;

    LogicalExpression* determinizeMostLikely(
        std::vector<ActionState> const& actionStates) override;

    void collectInitialInfo(bool& isProbabilistic,
                            bool& containsArithmeticFunction,
                            StateFluentSet& dependentStateFluents,
                            ActionFluentSet& dependentActionFluents) override;
    void calculateDomain(Domains const& domains, ActionState const& action,
                         std::set<double>& res) override;
    void calculateDomainAsInterval(Domains const& domains,
                                   ActionState const& action, double& minRes,
                                   double& maxRes) override;

    void evaluate(double& res, State const& current,
                  ActionState const& action) const override;
    void evaluateToPD(DiscretePD& res, State const& current,
                      ActionState const& action) const override;
    void evaluateToKleene(std::set<double>& res, KleeneState const& current,
                          ActionState const& action) const override;

    void determineBounds(ActionState const& action, double& minRes,
                         double& maxRes) const override;

    z3::expr toZ3Formula(CSP& csp, int actionIndex) const override;

    void print(std::ostream& out) const override;
};

/*****************************************************************
                          Unaries
*****************************************************************/

class Negation : public LogicalExpression {
public:
    LogicalExpression* expr;

    Negation(LogicalExpression* _expr) : expr(_expr) {}

    LogicalExpression* replaceQuantifier(
        Instantiations& replace, Instantiator* inst) override;
    LogicalExpression* instantiate(
        RDDLTask* task, Instantiations& replace) override;
    LogicalExpression* simplify(Simplifications& replace) override;

    LogicalExpression* determinizeMostLikely(
        std::vector<ActionState> const& actionStates) override;

    void collectInitialInfo(bool& isProbabilistic,
                            bool& containsArithmeticFunction,
                            StateFluentSet& dependentStateFluents,
                            ActionFluentSet& dependentActionFluents) override;
    void calculateDomain(Domains const& domains, ActionState const& action,
                         std::set<double>& res) override;
    void calculateDomainAsInterval(Domains const& domains,
                                   ActionState const& action, double& minRes,
                                   double& maxRes) override;

    void evaluate(double& res, State const& current,
                  ActionState const& action) const override;
    void evaluateToPD(DiscretePD& res, State const& current,
                      ActionState const& action) const override;
    void evaluateToKleene(std::set<double>& res, KleeneState const& current,
                          ActionState const& action) const override;

    void determineBounds(ActionState const& action, double& minRes,
                         double& maxRes) const override;

    z3::expr toZ3Formula(CSP& csp, int actionIndex) const override;

    void print(std::ostream& out) const override;
};

class ExponentialFunction : public LogicalExpression {
public:
    ExponentialFunction(LogicalExpression* _expr) : expr(_expr) {}

    LogicalExpression* expr;

    LogicalExpression* replaceQuantifier(
        Instantiations& replace, Instantiator* inst) override;
    LogicalExpression* instantiate(
        RDDLTask* task, Instantiations& replace) override;
    LogicalExpression* simplify(Simplifications& replace) override;

    LogicalExpression* determinizeMostLikely(
        std::vector<ActionState> const& actionStates) override;

    void collectInitialInfo(bool& isProbabilistic,
                            bool& containsArithmeticFunction,
                            StateFluentSet& dependentStateFluents,
                            ActionFluentSet& dependentActionFluents) override;
    void calculateDomain(Domains const& domains, ActionState const& action,
                         std::set<double>& res) override;
    void calculateDomainAsInterval(Domains const& domains,
                                   ActionState const& action, double& minRes,
                                   double& maxRes) override;

    void evaluate(double& res, State const& current,
                  ActionState const& action) const override;
    void evaluateToPD(DiscretePD& res, State const& current,
                      ActionState const& action) const override;
    void evaluateToKleene(std::set<double>& res, KleeneState const& current,
                          ActionState const& action) const override;

    void determineBounds(ActionState const& action, double& minRes,
                         double& maxRes) const override;

    void print(std::ostream& out) const override;
};

/*****************************************************************
                   Probability Distributions
*****************************************************************/

class KronDeltaDistribution : public LogicalExpression {
public:
    KronDeltaDistribution(LogicalExpression* _expr)
        : LogicalExpression(), expr(_expr) {}

    LogicalExpression* replaceQuantifier(
        Instantiations& replace, Instantiator* inst) override;
    LogicalExpression* instantiate(
        RDDLTask* task, Instantiations& replace) override;
    LogicalExpression* simplify(Simplifications& replace) override;

    LogicalExpression* expr;

    void print(std::ostream& out) const override;
};

class BernoulliDistribution : public LogicalExpression {
public:
    BernoulliDistribution(LogicalExpression* _expr)
        : LogicalExpression(), expr(_expr) {}

    LogicalExpression* replaceQuantifier(
        Instantiations& replace, Instantiator* inst) override;
    LogicalExpression* instantiate(
        RDDLTask* task, Instantiations& replace) override;
    LogicalExpression* simplify(Simplifications& replace) override;

    LogicalExpression* determinizeMostLikely(
        std::vector<ActionState> const& actionStates) override;

    void collectInitialInfo(bool& isProbabilistic,
                            bool& containsArithmeticFunction,
                            StateFluentSet& dependentStateFluents,
                            ActionFluentSet& dependentActionFluents) override;
    void calculateDomain(Domains const& domains, ActionState const& action,
                         std::set<double>& res) override;

    void calculateDomainAsInterval(Domains const& domains,
                                   ActionState const& action, double& minRes,
                                   double& maxRes) override;

    void evaluate(double& res, State const& current,
                  ActionState const& action) const override;
    void evaluateToPD(DiscretePD& res, State const& current,
                      ActionState const& action) const override;
    void evaluateToKleene(std::set<double>& res, KleeneState const& current,
                          ActionState const& action) const override;

    void determineBounds(ActionState const& action, double& minRes,
                         double& maxRes) const override;

    void print(std::ostream& out) const override;

    LogicalExpression* expr;
};

class DiscreteDistribution : public LogicalExpression {
public:
    std::vector<LogicalExpression*> values;
    std::vector<LogicalExpression*> probabilities;

    DiscreteDistribution() = default;
    DiscreteDistribution(std::vector<LogicalExpression*> _values,
                         std::vector<LogicalExpression*> _probabilities)
        : LogicalExpression(), values(_values), probabilities(_probabilities) {}

    LogicalExpression* replaceQuantifier(
        Instantiations& replace, Instantiator* inst) override;
    LogicalExpression* instantiate(
        RDDLTask* task, Instantiations& replace) override;
    LogicalExpression* simplify(Simplifications& replace) override;

    LogicalExpression* determinizeMostLikely(
        std::vector<ActionState> const& actionStates) override;

    void collectInitialInfo(bool& isProbabilistic,
                            bool& containsArithmeticFunction,
                            StateFluentSet& dependentStateFluents,
                            ActionFluentSet& dependentActionFluents) override;
    void calculateDomain(Domains const& domains, ActionState const& action,
                         std::set<double>& res) override;
    void calculateDomainAsInterval(Domains const& domains,
                                   ActionState const& action, double& minRes,
                                   double& maxRes) override;

    void evaluate(double& res, State const& current,
                  ActionState const& action) const override;
    void evaluateToPD(DiscretePD& res, State const& current,
                      ActionState const& action) const override;
    void evaluateToKleene(std::set<double>& res, KleeneState const& current,
                          ActionState const& action) const override;

    void determineBounds(ActionState const& action, double& minRes,
                         double& maxRes) const override;

    void print(std::ostream& out) const override;
};

/*****************************************************************
                         Conditionals
*****************************************************************/

class IfThenElseExpression : public LogicalExpression {
public:
    LogicalExpression* condition;
    LogicalExpression* valueIfTrue;
    LogicalExpression* valueIfFalse;

    IfThenElseExpression(LogicalExpression* _condition,
                         LogicalExpression* _valueIfTrue,
                         LogicalExpression* _valueIfFalse)
        : condition(_condition),
          valueIfTrue(_valueIfTrue),
          valueIfFalse(_valueIfFalse) {}

    LogicalExpression* replaceQuantifier(
        Instantiations& replace, Instantiator* inst) override;
    LogicalExpression* instantiate(
        RDDLTask* task, Instantiations& replace) override;
    LogicalExpression* simplify(Simplifications& replace) override;

    LogicalExpression* determinizeMostLikely(
        std::vector<ActionState> const& actionStates) override;

    void collectInitialInfo(bool& isProbabilistic,
                            bool& containsArithmeticFunction,
                            StateFluentSet& dependentStateFluents,
                            ActionFluentSet& dependentActionFluents) override;
    void calculateDomain(Domains const& domains, ActionState const& action,
                         std::set<double>& res) override;
    void calculateDomainAsInterval(Domains const& domains,
                                   ActionState const& action, double& minRes,
                                   double& maxRes) override;

    void evaluate(double& res, State const& current,
                  ActionState const& action) const override;
    void evaluateToPD(DiscretePD& res, State const& current,
                      ActionState const& action) const override;
    void evaluateToKleene(std::set<double>& res, KleeneState const& current,
                          ActionState const& action) const override;

    void determineBounds(ActionState const& action, double& minRes,
                         double& maxRes) const override;

    void print(std::ostream& out) const override;
};

class MultiConditionChecker : public LogicalExpression {
public:
    std::vector<LogicalExpression*> conditions;
    std::vector<LogicalExpression*> effects;

    MultiConditionChecker(std::vector<LogicalExpression*> _conditions,
                          std::vector<LogicalExpression*> _effects)
        : conditions(_conditions), effects(_effects) {}

    LogicalExpression* replaceQuantifier(
        Instantiations& replace, Instantiator* inst) override;
    LogicalExpression* instantiate(
        RDDLTask* task, Instantiations& replace) override;
    LogicalExpression* simplify(Simplifications& replace) override;

    LogicalExpression* determinizeMostLikely(
        std::vector<ActionState> const& actionStates) override;

    void collectInitialInfo(bool& isProbabilistic,
                            bool& containsArithmeticFunction,
                            StateFluentSet& dependentStateFluents,
                            ActionFluentSet& dependentActionFluents) override;
    void calculateDomain(Domains const& domains, ActionState const& action,
                         std::set<double>& res) override;
    void calculateDomainAsInterval(Domains const& domains,
                                   ActionState const& action, double& minRes,
                                   double& maxRes) override;

    void evaluate(double& res, State const& current,
                  ActionState const& action) const override;
    void evaluateToPD(DiscretePD& res, State const& current,
                      ActionState const& action) const override;
    void evaluateToKleene(std::set<double>& res, KleeneState const& current,
                          ActionState const& action) const override;

    void determineBounds(ActionState const& action, double& minRes,
                         double& maxRes) const override;

    z3::expr toZ3Formula(CSP& csp, int actionIndex) const override;

    void print(std::ostream& out) const override;

private:
    z3::expr buildZ3Formula(CSP& csp, int actionIndex, int index) const;
};

#endif
