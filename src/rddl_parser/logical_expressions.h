#ifndef LOGICAL_EXPRESSIONS_H
#define LOGICAL_EXPRESSIONS_H

#include <set>
#include <map>
#include <vector>
#include <string>

class Instantiator;
class PlanningTask;
class StateFluent;
class ActionFluent;
class NumericConstant;
class Object;
class State;
class ActionState;

class LogicalExpression {
public:
    virtual ~LogicalExpression() {}

    virtual LogicalExpression* replaceQuantifier(std::map<std::string, Object*>& replacements, Instantiator* instantiator);
    virtual LogicalExpression* instantiate(PlanningTask* task, std::map<std::string, Object*>& replacements);
    virtual LogicalExpression* simplify(std::map<StateFluent*, double>& replacements);
    virtual LogicalExpression* determinizeMostLikely(NumericConstant* randomNumberReplacement);
    virtual void collectInitialInfo(bool& isProbabilistic,
                                    bool& containsArithmeticFunction,
                                    std::set<StateFluent*>& dependentStateFluents,
                                    std::set<ActionFluent*>& dependentActionFluents);
    virtual void classifyActionFluents(std::set<ActionFluent*>& positiveDependentActionFluents, 
                                       std::set<ActionFluent*>& negativeDependentActionFluents);
    virtual void calculateDomain(std::vector<std::set<double> > const& domains, ActionState const& actions, std::set<double>& res);

    virtual void evaluate(double& res, State const& current, ActionState const& actions) const;

    virtual void print(std::ostream& out);
};

/*****************************************************************
                         Schematics
*****************************************************************/

class Type {
public:
    Type(std::string _name, Type* _superType = NULL) :
        name(_name), superType(_superType) {}

    std::string name;
    Type* superType;
    std::vector<Object*> objects;
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
    Parameter(std::string _name, Type* _type = NULL):
        name(_name), type(_type) {}

    std::string name;
    Type* type;

    LogicalExpression* replaceQuantifier(std::map<std::string, Object*>& replacements, Instantiator* instantiator);
    LogicalExpression* instantiate(PlanningTask* task, std::map<std::string, Object*>& replacements);
    void print(std::ostream& out);
};

class Object : public Parameter {
public:
    Object(std::string _name, Type* _type) :
        Parameter(_name, _type), types(), values() {}
    ~Object() {}

    // This was just one type and value, but each object can be referenced in
    // the context of each of its types in the type hierarchy and it may have a
    // different value for each type. As we only support enums so far, the size
    // of types and values must always be 1 as enums have only one type.
    std::vector<Type*> types;
    std::vector<double> values;

    LogicalExpression* replaceQuantifier(std::map<std::string, Object*>& replacements, Instantiator* instantiator);
    LogicalExpression* instantiate(PlanningTask* task, std::map<std::string, Object*>& replacements);
    void print(std::ostream& out);
};

class ParametrizedVariable : public LogicalExpression {
public:
    enum VariableType {
        STATE_FLUENT, 
        ACTION_FLUENT, 
        //DERIVED_FLUENT,
        //INTERM_FLUENT, 
        NON_FLUENT};

    ParametrizedVariable(std::string _variableName, std::vector<Parameter*> _params, VariableType _variableType, Type* _valueType, double _initialValue) :
        LogicalExpression(),
        variableName(_variableName),
        fullName(_variableName),
        params(_params),
        variableType(_variableType),
        valueType(_valueType),
        initialValue(_initialValue) {}
    ParametrizedVariable(ParametrizedVariable const& source, std::vector<Parameter*> _params);
    ParametrizedVariable(ParametrizedVariable const& source, std::vector<Parameter*> _params, double _initialValue);

    std::string variableName;
    std::string fullName;
    std::vector<Parameter*> params;
    VariableType variableType;
    Type* valueType;
    double initialValue;

    LogicalExpression* replaceQuantifier(std::map<std::string, Object*>& replacements, Instantiator* instantiator);
    LogicalExpression* instantiate(PlanningTask* task, std::map<std::string, Object*>& replacements);
    LogicalExpression* simplify(std::map<StateFluent*, double>& replacements);
    LogicalExpression* determinizeMostLikely(NumericConstant* randomNumberReplacement);
};

/*****************************************************************
                           Atomics
*****************************************************************/

class StateFluent : public ParametrizedVariable {
public:
    StateFluent(ParametrizedVariable const& source, std::vector<Parameter*> _params, double _initialValue) :
        ParametrizedVariable(source, _params, _initialValue), index(-1) {}
    StateFluent(ParametrizedVariable const& source, std::vector<Parameter*> _params) :
        ParametrizedVariable(source, _params), index(-1) {}

    int index;

    LogicalExpression* simplify(std::map<StateFluent*, double>& replacements);
    void collectInitialInfo(bool& isProbabilistic,
                            bool& containsArithmeticFunction,
                            std::set<StateFluent*>& dependentStateFluents,
                            std::set<ActionFluent*>& dependentActionFluents);
    void classifyActionFluents(std::set<ActionFluent*>& positiveDependentActionFluents, 
                               std::set<ActionFluent*>& negativeDependentActionFluents);
    void calculateDomain(std::vector<std::set<double> > const& domains, ActionState const& actions, std::set<double>& res);

    void evaluate(double& res, State const& current, ActionState const& actions) const;

    void print(std::ostream& out);
};

class ActionFluent : public ParametrizedVariable {
public:
    ActionFluent(ParametrizedVariable const& source, std::vector<Parameter*> _params) :
        ParametrizedVariable(source, _params, 0.0), index(-1) {}

    int index;

    void collectInitialInfo(bool& isProbabilistic,
                            bool& containsArithmeticFunction,
                            std::set<StateFluent*>& dependentStateFluents,
                            std::set<ActionFluent*>& dependentActionFluents);
    void classifyActionFluents(std::set<ActionFluent*>& positiveDependentActionFluents, 
                               std::set<ActionFluent*>& negativeDependentActionFluents);
    void calculateDomain(std::vector<std::set<double> > const& domains, ActionState const& actions, std::set<double>& res);

    void evaluate(double& res, State const& current, ActionState const& actions) const;

    void print(std::ostream& out);

    // This is used to sort ActionFluents by their name to ensure deterministic
    // behaviour
    struct ActionFluentSort {
        bool operator() (ActionFluent* const& lhs, ActionFluent* const& rhs) const {
            return lhs->fullName < rhs->fullName;
        }
    };
};

class NonFluent : public ParametrizedVariable {
public:
    NonFluent(ParametrizedVariable const& source, std::vector<Parameter*> _params, double _initialValue) :
        ParametrizedVariable(source, _params, _initialValue) {}
    NonFluent(ParametrizedVariable const& source, std::vector<Parameter*> _params) :
        ParametrizedVariable(source, _params) {}
};

class NumericConstant : public LogicalExpression {
public:
    NumericConstant(double _value) :
        value(_value) {}

    double value;

    LogicalExpression* replaceQuantifier(std::map<std::string, Object*>& replacements, Instantiator* instantiator);
    LogicalExpression* instantiate(PlanningTask* task, std::map<std::string, Object*>& replacements);
    LogicalExpression* simplify(std::map<StateFluent*, double>& replacements);
    LogicalExpression* determinizeMostLikely(NumericConstant* randomNumberReplacement);
    void collectInitialInfo(bool& isProbabilistic,
                            bool& containsArithmeticFunction,
                            std::set<StateFluent*>& dependentStateFluents,
                            std::set<ActionFluent*>& dependentActionFluents);
    void classifyActionFluents(std::set<ActionFluent*>& positiveDependentActionFluents, 
                               std::set<ActionFluent*>& negativeDependentActionFluents);
    void calculateDomain(std::vector<std::set<double> > const& domains, ActionState const& actions, std::set<double>& res);

    void evaluate(double& res, State const& current, ActionState const& actions) const;

    void print(std::ostream& out);
};

/*****************************************************************
                          Quantifier
*****************************************************************/

class ParameterList : public LogicalExpression {
public:
    ParameterList(std::vector<Parameter*> _params, std::vector<Type*> _types) :
        params(_params), types(_types) {}
    ParameterList() {}

    std::vector<Parameter*> params;
    std::vector<Type*> types;

    void print(std::ostream& out);
};

class Quantifier : public LogicalExpression {
public:
    Quantifier(ParameterList* _paramList, LogicalExpression* _expr) :
        paramList(_paramList), expr(_expr) {}

    ParameterList* paramList;
    LogicalExpression* expr;

    void getReplacements(std::vector<std::string>& parameterNames, std::vector<std::vector<Parameter*> >& replacements, Instantiator* instantiator);
};

class Sumation : public Quantifier {
public:
    Sumation(ParameterList* _paramList, LogicalExpression* _expr) :
        Quantifier(_paramList, _expr) {}

    LogicalExpression* replaceQuantifier(std::map<std::string, Object*>& replacements, Instantiator* instantiator);
};

class Product : public Quantifier {
public:
    Product(ParameterList* _paramList, LogicalExpression* _expr) :
        Quantifier(_paramList, _expr) {}

    LogicalExpression* replaceQuantifier(std::map<std::string, Object*>& replacements, Instantiator* instantiator);
};

class UniversalQuantification : public Quantifier {
public:
    UniversalQuantification(ParameterList* _paramList, LogicalExpression* _expr) :
        Quantifier(_paramList, _expr) {}

    LogicalExpression* replaceQuantifier(std::map<std::string, Object*>& replacements, Instantiator* instantiator);
};

class ExistentialQuantification : public Quantifier {
public:
    ExistentialQuantification(ParameterList* _paramList, LogicalExpression* _expr) :
        Quantifier(_paramList, _expr) {}

    LogicalExpression* replaceQuantifier(std::map<std::string, Object*>& replacements, Instantiator* instantiator);
};

/*****************************************************************
                           Connectives
*****************************************************************/

class Connective : public LogicalExpression {
public:
    std::vector<LogicalExpression*> exprs;

    Connective(std::vector<LogicalExpression*>& _exprs) :
        exprs(_exprs) {}

    void classifyActionFluents(std::set<ActionFluent*>& positiveDependentActionFluents,
                               std::set<ActionFluent*>& negativeDependentActionFluents);
    void print(std::ostream& out);
};

class Conjunction : public Connective {
public:
    Conjunction(std::vector<LogicalExpression*>& _exprs) :
        Connective(_exprs) {}

    LogicalExpression* instantiate(PlanningTask* task, std::map<std::string, Object*>& replacements);
    LogicalExpression* replaceQuantifier(std::map<std::string, Object*>& replacements, Instantiator* instantiator);
    LogicalExpression* simplify(std::map<StateFluent*, double>& replacements);
    LogicalExpression* determinizeMostLikely(NumericConstant* randomNumberReplacement);
    void collectInitialInfo(bool& isProbabilistic,
                            bool& containsArithmeticFunction,
                            std::set<StateFluent*>& dependentStateFluents,
                            std::set<ActionFluent*>& dependentActionFluents);
    void calculateDomain(std::vector<std::set<double> > const& domains, ActionState const& actions, std::set<double>& res);

    void evaluate(double& res, State const& current, ActionState const& actions) const;

    void print(std::ostream& out);
};

class Disjunction : public Connective {
public:
    Disjunction(std::vector<LogicalExpression*>& _exprs) :
        Connective(_exprs) {}

    LogicalExpression* instantiate(PlanningTask* task, std::map<std::string, Object*>& replacements);
    LogicalExpression* replaceQuantifier(std::map<std::string, Object*>& replacements, Instantiator* instantiator);
    LogicalExpression* simplify(std::map<StateFluent*, double>& replacements);
    LogicalExpression* determinizeMostLikely(NumericConstant* randomNumberReplacement);
    void collectInitialInfo(bool& isProbabilistic,
                            bool& containsArithmeticFunction,
                            std::set<StateFluent*>& dependentStateFluents,
                            std::set<ActionFluent*>& dependentActionFluents);
    void calculateDomain(std::vector<std::set<double> > const& domains, ActionState const& actions, std::set<double>& res);

    void evaluate(double& res, State const& current, ActionState const& actions) const;

    void print(std::ostream& out);
};

class EqualsExpression : public Connective {
public:
    EqualsExpression(std::vector<LogicalExpression*>& _exprs) :
        Connective(_exprs) {}

    LogicalExpression* instantiate(PlanningTask* task, std::map<std::string, Object*>& replacements);
    LogicalExpression* replaceQuantifier(std::map<std::string, Object*>& replacements, Instantiator* instantiator);
    LogicalExpression* simplify(std::map<StateFluent*, double>& replacements);
    LogicalExpression* determinizeMostLikely(NumericConstant* randomNumberReplacement);
    void collectInitialInfo(bool& isProbabilistic,
                            bool& containsArithmeticFunction,
                            std::set<StateFluent*>& dependentStateFluents,
                            std::set<ActionFluent*>& dependentActionFluents);
    void classifyActionFluents(std::set<ActionFluent*>& positiveDependentActionFluents,
                               std::set<ActionFluent*>& negativeDependentActionFluents);
    void calculateDomain(std::vector<std::set<double> > const& domains, ActionState const& actions, std::set<double>& res);

    void evaluate(double& res, State const& current, ActionState const& actions) const;

    void print(std::ostream& out);
};

class GreaterExpression : public Connective {
public:
    GreaterExpression(std::vector<LogicalExpression*>& _exprs) :
        Connective(_exprs) {}

    LogicalExpression* instantiate(PlanningTask* task, std::map<std::string, Object*>& replacements);
    LogicalExpression* replaceQuantifier(std::map<std::string, Object*>& replacements, Instantiator* instantiator);
    LogicalExpression* simplify(std::map<StateFluent*, double>& replacements);
    LogicalExpression* determinizeMostLikely(NumericConstant* randomNumberReplacement);
    void collectInitialInfo(bool& isProbabilistic,
                            bool& containsArithmeticFunction,
                            std::set<StateFluent*>& dependentStateFluents,
                            std::set<ActionFluent*>& dependentActionFluents);
    void classifyActionFluents(std::set<ActionFluent*>& positiveDependentActionFluents,
                               std::set<ActionFluent*>& negativeDependentActionFluents);
    void calculateDomain(std::vector<std::set<double> > const& domains, ActionState const& actions, std::set<double>& res);

    void evaluate(double& res, State const& current, ActionState const& actions) const;

    void print(std::ostream& out);
};

class LowerExpression : public Connective {
public:
    LowerExpression(std::vector<LogicalExpression*>& _exprs) :
        Connective(_exprs) {}

    LogicalExpression* instantiate(PlanningTask* task, std::map<std::string, Object*>& replacements);
    LogicalExpression* replaceQuantifier(std::map<std::string, Object*>& replacements, Instantiator* instantiator);
    LogicalExpression* simplify(std::map<StateFluent*, double>& replacements);
    LogicalExpression* determinizeMostLikely(NumericConstant* randomNumberReplacement);
    void collectInitialInfo(bool& isProbabilistic,
                            bool& containsArithmeticFunction,
                            std::set<StateFluent*>& dependentStateFluents,
                            std::set<ActionFluent*>& dependentActionFluents);
    void classifyActionFluents(std::set<ActionFluent*>& positiveDependentActionFluents,
                               std::set<ActionFluent*>& negativeDependentActionFluents);
    void calculateDomain(std::vector<std::set<double> > const& domains, ActionState const& actions, std::set<double>& res);

    void evaluate(double& res, State const& current, ActionState const& actions) const;

    void print(std::ostream& out);
};

class GreaterEqualsExpression : public Connective {
public:
    GreaterEqualsExpression(std::vector<LogicalExpression*>& _exprs) :
        Connective(_exprs) {}

    LogicalExpression* instantiate(PlanningTask* task, std::map<std::string, Object*>& replacements);
    LogicalExpression* replaceQuantifier(std::map<std::string, Object*>& replacements, Instantiator* instantiator);
    LogicalExpression* simplify(std::map<StateFluent*, double>& replacements);
    LogicalExpression* determinizeMostLikely(NumericConstant* randomNumberReplacement);
    void collectInitialInfo(bool& isProbabilistic,
                            bool& containsArithmeticFunction,
                            std::set<StateFluent*>& dependentStateFluents,
                            std::set<ActionFluent*>& dependentActionFluents);
    void classifyActionFluents(std::set<ActionFluent*>& positiveDependentActionFluents,
                               std::set<ActionFluent*>& negativeDependentActionFluents);
    void calculateDomain(std::vector<std::set<double> > const& domains, ActionState const& actions, std::set<double>& res);

    void evaluate(double& res, State const& current, ActionState const& actions) const;

    void print(std::ostream& out);
};

class LowerEqualsExpression : public Connective {
public:
    LowerEqualsExpression(std::vector<LogicalExpression*>& _exprs) :
        Connective(_exprs) {}

    LogicalExpression* instantiate(PlanningTask* task, std::map<std::string, Object*>& replacements);
    LogicalExpression* replaceQuantifier(std::map<std::string, Object*>& replacements, Instantiator* instantiator);
    LogicalExpression* simplify(std::map<StateFluent*, double>& replacements);
    LogicalExpression* determinizeMostLikely(NumericConstant* randomNumberReplacement);
    void collectInitialInfo(bool& isProbabilistic,
                            bool& containsArithmeticFunction,
                            std::set<StateFluent*>& dependentStateFluents,
                            std::set<ActionFluent*>& dependentActionFluents);
    void classifyActionFluents(std::set<ActionFluent*>& positiveDependentActionFluents,
                               std::set<ActionFluent*>& negativeDependentActionFluents);
    void calculateDomain(std::vector<std::set<double> > const& domains, ActionState const& actions, std::set<double>& res);

    void evaluate(double& res, State const& current, ActionState const& actions) const;

    void print(std::ostream& out);
};

class Addition : public Connective {
public:
    Addition(std::vector<LogicalExpression*>& _exprs) :
        Connective(_exprs) {}

    LogicalExpression* instantiate(PlanningTask* task, std::map<std::string, Object*>& replacements);
    LogicalExpression* replaceQuantifier(std::map<std::string, Object*>& replacements, Instantiator* instantiator);
    LogicalExpression* simplify(std::map<StateFluent*, double>& replacements);
    LogicalExpression* determinizeMostLikely(NumericConstant* randomNumberReplacement);
    void collectInitialInfo(bool& isProbabilistic,
                            bool& containsArithmeticFunction,
                            std::set<StateFluent*>& dependentStateFluents,
                            std::set<ActionFluent*>& dependentActionFluents);
    void calculateDomain(std::vector<std::set<double> > const& domains, ActionState const& actions, std::set<double>& res);

    void evaluate(double& res, State const& current, ActionState const& actions) const;

    void print(std::ostream& out);
};

class Subtraction : public Connective {
public:
    Subtraction(std::vector<LogicalExpression*>& _exprs) :
        Connective(_exprs) {}

    LogicalExpression* instantiate(PlanningTask* task, std::map<std::string, Object*>& replacements);
    LogicalExpression* replaceQuantifier(std::map<std::string, Object*>& replacements, Instantiator* instantiator);
    LogicalExpression* simplify(std::map<StateFluent*, double>& replacements);
    LogicalExpression* determinizeMostLikely(NumericConstant* randomNumberReplacement);
    void collectInitialInfo(bool& isProbabilistic,
                            bool& containsArithmeticFunction,
                            std::set<StateFluent*>& dependentStateFluents,
                            std::set<ActionFluent*>& dependentActionFluents);
    void classifyActionFluents(std::set<ActionFluent*>& positiveDependentActionFluents,
                               std::set<ActionFluent*>& negativeDependentActionFluents);
    void calculateDomain(std::vector<std::set<double> > const& domains, ActionState const& actions, std::set<double>& res);

    void evaluate(double& res, State const& current, ActionState const& actions) const;

    void print(std::ostream& out);
};

class Multiplication : public Connective {
public:
    Multiplication(std::vector<LogicalExpression*>& _exprs) :
        Connective(_exprs) {}

    LogicalExpression* instantiate(PlanningTask* task, std::map<std::string, Object*>& replacements);
    LogicalExpression* replaceQuantifier(std::map<std::string, Object*>& replacements, Instantiator* instantiator);
    LogicalExpression* simplify(std::map<StateFluent*, double>& replacements);
    LogicalExpression* determinizeMostLikely(NumericConstant* randomNumberReplacement);
    void collectInitialInfo(bool& isProbabilistic,
                            bool& containsArithmeticFunction,
                            std::set<StateFluent*>& dependentStateFluents,
                            std::set<ActionFluent*>& dependentActionFluents);
    void classifyActionFluents(std::set<ActionFluent*>& positiveDependentActionFluents,
                               std::set<ActionFluent*>& negativeDependentActionFluents);
    void calculateDomain(std::vector<std::set<double> > const& domains, ActionState const& actions, std::set<double>& res);

    void evaluate(double& res, State const& current, ActionState const& actions) const;

    void print(std::ostream& out);
};

class Division : public Connective {
public:
    Division(std::vector<LogicalExpression*>& _exprs) :
        Connective(_exprs) {}

    LogicalExpression* instantiate(PlanningTask* task, std::map<std::string, Object*>& replacements);
    LogicalExpression* replaceQuantifier(std::map<std::string, Object*>& replacements, Instantiator* instantiator);
    LogicalExpression* simplify(std::map<StateFluent*, double>& replacements);
    LogicalExpression* determinizeMostLikely(NumericConstant* randomNumberReplacement);
    void collectInitialInfo(bool& isProbabilistic,
                            bool& containsArithmeticFunction,
                            std::set<StateFluent*>& dependentStateFluents,
                            std::set<ActionFluent*>& dependentActionFluents);
    void classifyActionFluents(std::set<ActionFluent*>& positiveDependentActionFluents,
                               std::set<ActionFluent*>& negativeDependentActionFluents);
    void calculateDomain(std::vector<std::set<double> > const& domains, ActionState const& actions, std::set<double>& res);

    void evaluate(double& res, State const& current, ActionState const& actions) const;

    void print(std::ostream& out);
};

/*****************************************************************
                          Unaries
*****************************************************************/

class Negation : public LogicalExpression {
public:
    LogicalExpression* expr;

    Negation(LogicalExpression* _expr) :
        expr(_expr) {}

    LogicalExpression* replaceQuantifier(std::map<std::string, Object*>& replacements, Instantiator* instantiator);
    LogicalExpression* instantiate(PlanningTask* task, std::map<std::string, Object*>& replacements);
    LogicalExpression* simplify(std::map<StateFluent*, double>& replacements);
    LogicalExpression* determinizeMostLikely(NumericConstant* randomNumberReplacement);
    void collectInitialInfo(bool& isProbabilistic,
                            bool& containsArithmeticFunction,
                            std::set<StateFluent*>& dependentStateFluents,
                            std::set<ActionFluent*>& dependentActionFluents);
    void classifyActionFluents(std::set<ActionFluent*>& positiveDependentActionFluents,
                               std::set<ActionFluent*>& negativeDependentActionFluents);
    void calculateDomain(std::vector<std::set<double> > const& domains, ActionState const& actions, std::set<double>& res);

    void evaluate(double& res, State const& current, ActionState const& actions) const;

    void print(std::ostream& out);
};

/*****************************************************************
                   Probability Distributions
*****************************************************************/

class KronDeltaDistribution : public LogicalExpression {
public:
    KronDeltaDistribution(LogicalExpression* _expr) :
        LogicalExpression(),
        expr(_expr) {}

    LogicalExpression* instantiate(PlanningTask* task, std::map<std::string, Object*>& replacements);
    LogicalExpression* replaceQuantifier(std::map<std::string, Object*>& replacements, Instantiator* instantiator);
    LogicalExpression* simplify(std::map<StateFluent*, double>& replacements);

    LogicalExpression* expr;
};

class BernoulliDistribution : public LogicalExpression {
public:
    BernoulliDistribution(LogicalExpression* _expr) :
        LogicalExpression(),
        expr(_expr) {}

    LogicalExpression* instantiate(PlanningTask* task, std::map<std::string, Object*>& replacements);
    LogicalExpression* replaceQuantifier(std::map<std::string, Object*>& replacements, Instantiator* instantiator);
    LogicalExpression* simplify(std::map<StateFluent*, double>& replacements);
    LogicalExpression* determinizeMostLikely(NumericConstant* randomNumberReplacement);
    void collectInitialInfo(bool& isProbabilistic,
                            bool& containsArithmeticFunction,
                            std::set<StateFluent*>& dependentStateFluents,
                            std::set<ActionFluent*>& dependentActionFluents);
    void calculateDomain(std::vector<std::set<double> > const& domains, ActionState const& actions, std::set<double>& res);

    void evaluate(double& res, State const& current, ActionState const& actions) const;

    void print(std::ostream& out);

    LogicalExpression* expr;
};

class DiscreteDistribution : public LogicalExpression {
public:
    std::vector<LogicalExpression*> values;
    std::vector<LogicalExpression*> probabilities;
    
    DiscreteDistribution(std::vector<LogicalExpression*> _values, std::vector<LogicalExpression*> _probabilities) :
    	LogicalExpression(), values(_values), probabilities(_probabilities) {}

    LogicalExpression* replaceQuantifier(std::map<std::string, Object*>& replacements, Instantiator* instantiator);
    LogicalExpression* instantiate(PlanningTask* task, std::map<std::string, Object*>& replacements);
    LogicalExpression* simplify(std::map<StateFluent*, double>& replacements);
    LogicalExpression* determinizeMostLikely(NumericConstant* randomNumberReplacement);
    void collectInitialInfo(bool& isProbabilistic,
                            bool& containsArithmeticFunction,
                            std::set<StateFluent*>& dependentStateFluents,
                            std::set<ActionFluent*>& dependentActionFluents);
    void calculateDomain(std::vector<std::set<double> > const& domains, ActionState const& actions, std::set<double>& res);

    void evaluate(double& res, State const& current, ActionState const& actions) const;

    void print(std::ostream& out);
};

/*****************************************************************
                         Conditionals
*****************************************************************/

class IfThenElseExpression : public LogicalExpression {
public:
    LogicalExpression* condition;
    LogicalExpression* valueIfTrue;
    LogicalExpression* valueIfFalse;

    IfThenElseExpression(LogicalExpression* _condition, LogicalExpression* _valueIfTrue, LogicalExpression* _valueIfFalse) :
        condition(_condition), valueIfTrue(_valueIfTrue), valueIfFalse(_valueIfFalse) {}

    LogicalExpression* replaceQuantifier(std::map<std::string, Object*>& replacements, Instantiator* instantiator);
    LogicalExpression* instantiate(PlanningTask* task, std::map<std::string, Object*>& replacements);
    LogicalExpression* simplify(std::map<StateFluent*, double>& replacements);
    LogicalExpression* determinizeMostLikely(NumericConstant* randomNumberReplacement);
    void collectInitialInfo(bool& isProbabilistic,
                            bool& containsArithmeticFunction,
                            std::set<StateFluent*>& dependentStateFluents,
                            std::set<ActionFluent*>& dependentActionFluents);
    void classifyActionFluents(std::set<ActionFluent*>& positiveDependentActionFluents,
                               std::set<ActionFluent*>& negativeDependentActionFluents);
    void calculateDomain(std::vector<std::set<double> > const& domains, ActionState const& actions, std::set<double>& res);

    void evaluate(double& res, State const& current, ActionState const& actions) const;

    void print(std::ostream& out);
};

class MultiConditionChecker : public LogicalExpression {
public:
    std::vector<LogicalExpression*> conditions;
    std::vector<LogicalExpression*> effects;

    MultiConditionChecker(std::vector<LogicalExpression*> _conditions, std::vector<LogicalExpression*> _effects) :
        conditions(_conditions), effects(_effects) {}

    LogicalExpression* replaceQuantifier(std::map<std::string, Object*>& replacements, Instantiator* instantiator);
    LogicalExpression* instantiate(PlanningTask* task, std::map<std::string, Object*>& replacements);
    LogicalExpression* simplify(std::map<StateFluent*, double>& replacements);
    LogicalExpression* determinizeMostLikely(NumericConstant* randomNumberReplacement);
    void collectInitialInfo(bool& isProbabilistic,
                            bool& containsArithmeticFunction,
                            std::set<StateFluent*>& dependentStateFluents,
                            std::set<ActionFluent*>& dependentActionFluents);
    void classifyActionFluents(std::set<ActionFluent*>& positiveDependentActionFluents,
                               std::set<ActionFluent*>& negativeDependentActionFluents);
    void calculateDomain(std::vector<std::set<double> > const& domains, ActionState const& actions, std::set<double>& res);

    void evaluate(double& res, State const& current, ActionState const& actions) const;

    void print(std::ostream& out);
};

#endif
