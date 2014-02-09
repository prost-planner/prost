#ifndef LOGICAL_EXPRESSIONS_H
#define LOGICAL_EXPRESSIONS_H

#include "typed_objects.h"
#include "kleene_state.h"
#include "actions.h"
#include "probability_distribution.h"

#include <set>

class RDDLParser;
class Instantiator;

class LogicalExpression {
public:
    virtual ~LogicalExpression() {}

    virtual LogicalExpression* replaceQuantifier(UnprocessedPlanningTask* task, std::map<std::string, std::string>& replacements, Instantiator* instantiator);
    virtual LogicalExpression* instantiate(UnprocessedPlanningTask* task, std::map<std::string, Object*>& replacements);
    virtual LogicalExpression* simplify(UnprocessedPlanningTask* task, std::map<StateFluent*,NumericConstant*>& replacements);

    virtual LogicalExpression* determinizeMostLikely(NumericConstant* randomNumberReplacement);

    virtual void collectInitialInfo(bool& /*isProbabilistic*/, bool& /*containsArithmeticFunction*/, std::set<StateFluent*>& /*dependentStateFluents*/, 
                                    std::set<ActionFluent*>& /*positiveDependentActionFluents*/, std::set<ActionFluent*>& /*negativeDependentActionFluents*/) {}
    virtual void calculateDomain(ActionState const& actions, std::set<double>& res);
    virtual void calculateProbDomain(ActionState const& actions, std::set<double>& res);

    virtual void evaluate(double& res, State const& current, ActionState const& actions);
    virtual void evaluateToPD(DiscretePD& res, State const& current, ActionState const& actions);
    virtual void evaluateToKleene(std::set<double>& res, KleeneState const& current, ActionState const& actions);

    virtual void print(std::ostream& out) = 0;
};

/*****************************************************************
                         Schematics
*****************************************************************/

class VariableDefinition {
public:
    static void parse(std::string& desc, UnprocessedPlanningTask* task);

    static VariableDefinition* rewardInstance() {
        static VariableDefinition* rewardInst = new VariableDefinition("reward", std::vector<ObjectType*>(), VariableDefinition::STATE_FLUENT, RealType::instance());
        return rewardInst;
    }

    std::string name;
    std::vector<ObjectType*> params;

    enum VariableType {
        STATE_FLUENT, 
        ACTION_FLUENT, 
        INTERM_FLUENT, 
        NON_FLUENT} variableType;

    Type* valueType;
    double defaultValue;
    int level;

    void print(std::ostream& out);

private:
    VariableDefinition(std::string _name, std::vector<ObjectType*> _params, VariableType _variableType, Type* _valueType, double _defaultValue = 0.0, int _level = 1) :
        name(_name), params(_params), variableType(_variableType), valueType(_valueType), defaultValue(_defaultValue), level(_level) {}
};

class ParameterDefinition : public LogicalExpression {
public:
    std::string parameterName;
    ObjectType* parameterType;

    ParameterDefinition(std::string _parameterName, ObjectType* _parameterType):
        parameterName(_parameterName), parameterType(_parameterType) {}

    void print(std::ostream& out);
};

class ParameterDefinitionSet : public LogicalExpression {
public:
    std::vector<ParameterDefinition*> parameterDefs;

    ParameterDefinitionSet(std::vector<ParameterDefinition*>& _defs) :
        parameterDefs(_defs) {}
    ParameterDefinitionSet() {}

    void print(std::ostream& out);
};

class UninstantiatedVariable : public LogicalExpression {
public:
    UninstantiatedVariable(VariableDefinition* _parent, std::vector<std::string> _params);

    VariableDefinition* parent;

    std::string name;
    std::vector<std::string> params;

    LogicalExpression* replaceQuantifier(UnprocessedPlanningTask* task, std::map<std::string, std::string>& replacements, Instantiator* instantiator);
    LogicalExpression* instantiate(UnprocessedPlanningTask* task, std::map<std::string, Object*>& replacements);

    void print(std::ostream& out);
};

/*****************************************************************
                           Atomics
*****************************************************************/

class AtomicLogicalExpression : public LogicalExpression {
public:
    static void parse(std::string& desc, UnprocessedPlanningTask* task);

    VariableDefinition* parent;
    std::string name;
    std::vector<Object*> params;

    double initialValue;
    int index;

    LogicalExpression* instantiate(UnprocessedPlanningTask* task, std::map<std::string, Object*>& replacements);
    LogicalExpression* simplify(UnprocessedPlanningTask* task, std::map<StateFluent*,NumericConstant*>& replacements);
    LogicalExpression* determinizeMostLikely(NumericConstant* randomNumberReplacement);

    void print(std::ostream& out);

protected:
    AtomicLogicalExpression(VariableDefinition* _parent, std::vector<Object*> _params, double _initialValue);
};

class StateFluent : public AtomicLogicalExpression {
public:
    static StateFluent* rewardInstance() {
        static StateFluent* rewardInst = new StateFluent(VariableDefinition::rewardInstance(), std::vector<Object*>());
        return rewardInst;
    }

    StateFluent(VariableDefinition* _parent, std::vector<Object*> _params, double _initialValue) :
        AtomicLogicalExpression(_parent, _params, _initialValue) {}
    StateFluent(VariableDefinition* _parent, std::vector<Object*> _params) :
        AtomicLogicalExpression(_parent, _params, _parent->defaultValue) {}

    LogicalExpression* simplify(UnprocessedPlanningTask* task, std::map<StateFluent*,NumericConstant*>& replacements);

    void collectInitialInfo(bool& isProbabilistic, bool& containsArithmeticFunction, std::set<StateFluent*>& dependentStateFluents, 
                            std::set<ActionFluent*>& positiveDependentActionFluents, std::set<ActionFluent*>& negativeDependentActionFluents);
    void calculateDomain(ActionState const& actions, std::set<double>& res);
    void calculateProbDomain(ActionState const& actions, std::set<double>& res);

    void evaluate(double& res, State const& current, ActionState const& actions);
    void evaluateToPD(DiscretePD& res, State const& current, ActionState const& actions);
    void evaluateToKleene(std::set<double>& res, KleeneState const& current, ActionState const& actions);
};

class ActionFluent : public AtomicLogicalExpression {
public:
    ActionFluent(VariableDefinition* _parent, std::vector<Object*> _params) :
        AtomicLogicalExpression(_parent, _params, 0.0) {}

    void collectInitialInfo(bool& isProbabilistic, bool& containsArithmeticFunction, std::set<StateFluent*>& dependentStateFluents, 
                            std::set<ActionFluent*>& positiveDependentActionFluents, std::set<ActionFluent*>& negativeDependentActionFluents);
    void calculateDomain(ActionState const& actions, std::set<double>& res);
    void calculateProbDomain(ActionState const& actions, std::set<double>& res);

    void evaluate(double& res, State const& current, ActionState const& actions);
    void evaluateToPD(DiscretePD& res, State const& current, ActionState const& actions);
    void evaluateToKleene(std::set<double>& res, KleeneState const& current, ActionState const& actions);
};

class NonFluent : public AtomicLogicalExpression {
public:
    NonFluent(VariableDefinition* _parent, std::vector<Object*> _params, double _initialValue) :
        AtomicLogicalExpression(_parent, _params, _initialValue) {}
    NonFluent(VariableDefinition* _parent, std::vector<Object*> _params) :
        AtomicLogicalExpression(_parent, _params, _parent->defaultValue) {}
};

class NumericConstant : public LogicalExpression {
public:
    static NumericConstant* truth() {
        static NumericConstant* truthInst = new NumericConstant(1.0);
        return truthInst;
    }

    static NumericConstant* falsity() {
        static NumericConstant* falsityInst = new NumericConstant(0.0);
        return falsityInst;
    }

    double value;

    NumericConstant(double _value) :
        value(_value) {}

    LogicalExpression* replaceQuantifier(UnprocessedPlanningTask* task, std::map<std::string, std::string>& replacements, Instantiator* instantiator);
    LogicalExpression* instantiate(UnprocessedPlanningTask* task, std::map<std::string, Object*>& replacements);
    LogicalExpression* simplify(UnprocessedPlanningTask* task, std::map<StateFluent*,NumericConstant*>& replacements);

    LogicalExpression* determinizeMostLikely(NumericConstant* randomNumberReplacement);

    void calculateDomain(ActionState const& actions, std::set<double>& res);
    void calculateProbDomain(ActionState const& actions, std::set<double>& res);

    void evaluate(double& res, State const& current, ActionState const& actions);
    void evaluateToPD(DiscretePD& res, State const& current, ActionState const& actions);
    void evaluateToKleene(std::set<double>& res, KleeneState const& current, ActionState const& actions);

    void print(std::ostream& out);
};

/*****************************************************************
                          Quantifier
*****************************************************************/

class Quantifier : public LogicalExpression {
public:
    ParameterDefinitionSet* parameterDefsSet;
    LogicalExpression* expr;

    Quantifier(std::vector<LogicalExpression*>& _exprs);

    void getReplacements(UnprocessedPlanningTask* task, std::vector<std::string>& parameterNames, std::vector<std::vector<Object*> >& replacements, Instantiator* instantiator);

    void print(std::ostream& out);
};

class Sumation : public Quantifier {
public:
    Sumation(std::vector<LogicalExpression*>& _exprs) :
        Quantifier(_exprs) {}

    LogicalExpression* replaceQuantifier(UnprocessedPlanningTask* task, std::map<std::string, std::string>& replacements, Instantiator* instantiator);

    void print(std::ostream& out);
};

class Product : public Quantifier {
public:
    Product(std::vector<LogicalExpression*>& _exprs) :
        Quantifier(_exprs) {}

    LogicalExpression* replaceQuantifier(UnprocessedPlanningTask* task, std::map<std::string, std::string>& replacements, Instantiator* instantiator);

    void print(std::ostream& out);    
};

class UniversalQuantification : public Quantifier {
public:
    UniversalQuantification(std::vector<LogicalExpression*>& _exprs) :
        Quantifier(_exprs) {}

    LogicalExpression* replaceQuantifier(UnprocessedPlanningTask* task, std::map<std::string, std::string>& replacements, Instantiator* instantiator);

    void print(std::ostream& out);
};

class ExistentialQuantification: public Quantifier {
public:
    ExistentialQuantification(std::vector<LogicalExpression*>& _exprs) :
        Quantifier(_exprs) {}

    LogicalExpression* replaceQuantifier(UnprocessedPlanningTask* task, std::map<std::string, std::string>& replacements, Instantiator* instantiator);

    void print(std::ostream& out);
};

/*****************************************************************
                           Connectives
*****************************************************************/

class Connective : public LogicalExpression {
public:
    std::vector<LogicalExpression*> exprs;

    Connective(std::vector<LogicalExpression*>& _exprs) :
        exprs(_exprs) {}

    void print(std::ostream& out);
};

class Conjunction : public Connective {
public:
    Conjunction(std::vector<LogicalExpression*>& _exprs) :
        Connective(_exprs) {}

    LogicalExpression* instantiate(UnprocessedPlanningTask* task, std::map<std::string, Object*>& replacements);
    LogicalExpression* replaceQuantifier(UnprocessedPlanningTask* task, std::map<std::string, std::string>& replacements, Instantiator* instantiator);
    LogicalExpression* simplify(UnprocessedPlanningTask* task, std::map<StateFluent*,NumericConstant*>& replacements);

    LogicalExpression* determinizeMostLikely(NumericConstant* randomNumberReplacement);

    void collectInitialInfo(bool& isProbabilistic, bool& containsArithmeticFunction, std::set<StateFluent*>& dependentStateFluents, 
                                    std::set<ActionFluent*>& positiveDependentActionFluents, std::set<ActionFluent*>& negativeDependentActionFluents);
    void calculateDomain(ActionState const& actions, std::set<double>& res);
    void calculateProbDomain(ActionState const& actions, std::set<double>& res);

    void evaluate(double& res, State const& current, ActionState const& actions);
    void evaluateToPD(DiscretePD& res, State const& current, ActionState const& actions);
    void evaluateToKleene(std::set<double>& res, KleeneState const& current, ActionState const& actions);

    void print(std::ostream& out);
};

class Disjunction : public Connective {
public:
    Disjunction(std::vector<LogicalExpression*>& _exprs) :
        Connective(_exprs) {}

    LogicalExpression* instantiate(UnprocessedPlanningTask* task, std::map<std::string, Object*>& replacements);
    LogicalExpression* replaceQuantifier(UnprocessedPlanningTask* task, std::map<std::string, std::string>& replacements, Instantiator* instantiator);
    LogicalExpression* simplify(UnprocessedPlanningTask* task, std::map<StateFluent*,NumericConstant*>& replacements);

    LogicalExpression* determinizeMostLikely(NumericConstant* randomNumberReplacement);

    void collectInitialInfo(bool& isProbabilistic, bool& containsArithmeticFunction, std::set<StateFluent*>& dependentStateFluents, 
                                    std::set<ActionFluent*>& positiveDependentActionFluents, std::set<ActionFluent*>& negativeDependentActionFluents);
    void calculateDomain(ActionState const& actions, std::set<double>& res);
    void calculateProbDomain(ActionState const& actions, std::set<double>& res);

    void evaluate(double& res, State const& current, ActionState const& actions);
    void evaluateToPD(DiscretePD& res, State const& current, ActionState const& actions);
    void evaluateToKleene(std::set<double>& res, KleeneState const& current, ActionState const& actions);

    void print(std::ostream& out);
};

class EqualsExpression : public Connective {
public:
    EqualsExpression(std::vector<LogicalExpression*>& _exprs) :
        Connective(_exprs) {}

    LogicalExpression* instantiate(UnprocessedPlanningTask* task, std::map<std::string, Object*>& replacements);
    LogicalExpression* replaceQuantifier(UnprocessedPlanningTask* task, std::map<std::string, std::string>& replacements, Instantiator* instantiator);
    LogicalExpression* simplify(UnprocessedPlanningTask* task, std::map<StateFluent*,NumericConstant*>& replacements);

    LogicalExpression* determinizeMostLikely(NumericConstant* randomNumberReplacement);

    void collectInitialInfo(bool& isProbabilistic, bool& containsArithmeticFunction, std::set<StateFluent*>& dependentStateFluents, 
                                    std::set<ActionFluent*>& positiveDependentActionFluents, std::set<ActionFluent*>& negativeDependentActionFluents);
    void calculateDomain(ActionState const& actions, std::set<double>& res);
    void calculateProbDomain(ActionState const& actions, std::set<double>& res);

    void evaluate(double& res, State const& current, ActionState const& actions);
    void evaluateToPD(DiscretePD& res, State const& current, ActionState const& actions);
    void evaluateToKleene(std::set<double>& res, KleeneState const& current, ActionState const& actions);

    void print(std::ostream& out);
};

class GreaterExpression : public Connective {
public:
    GreaterExpression(std::vector<LogicalExpression*>& _exprs) :
        Connective(_exprs) {}

    LogicalExpression* instantiate(UnprocessedPlanningTask* task, std::map<std::string, Object*>& replacements);
    LogicalExpression* replaceQuantifier(UnprocessedPlanningTask* task, std::map<std::string, std::string>& replacements, Instantiator* instantiator);
    LogicalExpression* simplify(UnprocessedPlanningTask* task, std::map<StateFluent*,NumericConstant*>& replacements);

    LogicalExpression* determinizeMostLikely(NumericConstant* randomNumberReplacement);

    void collectInitialInfo(bool& isProbabilistic, bool& containsArithmeticFunction, std::set<StateFluent*>& dependentStateFluents, 
                                    std::set<ActionFluent*>& positiveDependentActionFluents, std::set<ActionFluent*>& negativeDependentActionFluents);
    void calculateDomain(ActionState const& actions, std::set<double>& res);
    void calculateProbDomain(ActionState const& actions, std::set<double>& res);

    void evaluate(double& res, State const& current, ActionState const& actions);
    void evaluateToPD(DiscretePD& res, State const& current, ActionState const& actions);
    void evaluateToKleene(std::set<double>& res, KleeneState const& current, ActionState const& actions);

    void print(std::ostream& out);
};

class LowerExpression : public Connective {
public:
    LowerExpression(std::vector<LogicalExpression*>& _exprs) :
        Connective(_exprs) {}

    LogicalExpression* instantiate(UnprocessedPlanningTask* task, std::map<std::string, Object*>& replacements);
    LogicalExpression* replaceQuantifier(UnprocessedPlanningTask* task, std::map<std::string, std::string>& replacements, Instantiator* instantiator);
    LogicalExpression* simplify(UnprocessedPlanningTask* task, std::map<StateFluent*,NumericConstant*>& replacements);

    LogicalExpression* determinizeMostLikely(NumericConstant* randomNumberReplacement);

    void collectInitialInfo(bool& isProbabilistic, bool& containsArithmeticFunction, std::set<StateFluent*>& dependentStateFluents, 
                                    std::set<ActionFluent*>& positiveDependentActionFluents, std::set<ActionFluent*>& negativeDependentActionFluents);
    void calculateDomain(ActionState const& actions, std::set<double>& res);
    void calculateProbDomain(ActionState const& actions, std::set<double>& res);

    void evaluate(double& res, State const& current, ActionState const& actions);
    void evaluateToPD(DiscretePD& res, State const& current, ActionState const& actions);
    void evaluateToKleene(std::set<double>& res, KleeneState const& current, ActionState const& actions);

    void print(std::ostream& out);
};

class GreaterEqualsExpression : public Connective {
public:
    GreaterEqualsExpression(std::vector<LogicalExpression*>& _exprs) :
        Connective(_exprs) {}

    LogicalExpression* instantiate(UnprocessedPlanningTask* task, std::map<std::string, Object*>& replacements);
    LogicalExpression* replaceQuantifier(UnprocessedPlanningTask* task, std::map<std::string, std::string>& replacements, Instantiator* instantiator);
    LogicalExpression* simplify(UnprocessedPlanningTask* task, std::map<StateFluent*,NumericConstant*>& replacements);

    LogicalExpression* determinizeMostLikely(NumericConstant* randomNumberReplacement);

    void collectInitialInfo(bool& isProbabilistic, bool& containsArithmeticFunction, std::set<StateFluent*>& dependentStateFluents, 
                                    std::set<ActionFluent*>& positiveDependentActionFluents, std::set<ActionFluent*>& negativeDependentActionFluents);
    void calculateDomain(ActionState const& actions, std::set<double>& res);
    void calculateProbDomain(ActionState const& actions, std::set<double>& res);

    void evaluate(double& res, State const& current, ActionState const& actions);
    void evaluateToPD(DiscretePD& res, State const& current, ActionState const& actions);
    void evaluateToKleene(std::set<double>& res, KleeneState const& current, ActionState const& actions);

    void print(std::ostream& out);
};

class LowerEqualsExpression : public Connective {
public:
    LowerEqualsExpression(std::vector<LogicalExpression*>& _exprs) :
        Connective(_exprs) {}

    LogicalExpression* instantiate(UnprocessedPlanningTask* task, std::map<std::string, Object*>& replacements);
    LogicalExpression* replaceQuantifier(UnprocessedPlanningTask* task, std::map<std::string, std::string>& replacements, Instantiator* instantiator);
    LogicalExpression* simplify(UnprocessedPlanningTask* task, std::map<StateFluent*,NumericConstant*>& replacements);

    LogicalExpression* determinizeMostLikely(NumericConstant* randomNumberReplacement);

    void collectInitialInfo(bool& isProbabilistic, bool& containsArithmeticFunction, std::set<StateFluent*>& dependentStateFluents, 
                                    std::set<ActionFluent*>& positiveDependentActionFluents, std::set<ActionFluent*>& negativeDependentActionFluents);
    void calculateDomain(ActionState const& actions, std::set<double>& res);
    void calculateProbDomain(ActionState const& actions, std::set<double>& res);

    void evaluate(double& res, State const& current, ActionState const& actions);
    void evaluateToPD(DiscretePD& res, State const& current, ActionState const& actions);
    void evaluateToKleene(std::set<double>& res, KleeneState const& current, ActionState const& actions);

    void print(std::ostream& out);
};

class Addition : public Connective {
public:
    Addition(std::vector<LogicalExpression*>& _exprs) :
        Connective(_exprs) {}

    LogicalExpression* instantiate(UnprocessedPlanningTask* task, std::map<std::string, Object*>& replacements);
    LogicalExpression* replaceQuantifier(UnprocessedPlanningTask* task, std::map<std::string, std::string>& replacements, Instantiator* instantiator);
    LogicalExpression* simplify(UnprocessedPlanningTask* task, std::map<StateFluent*,NumericConstant*>& replacements);

    LogicalExpression* determinizeMostLikely(NumericConstant* randomNumberReplacement);

    void collectInitialInfo(bool& isProbabilistic, bool& containsArithmeticFunction, std::set<StateFluent*>& dependentStateFluents, 
                                    std::set<ActionFluent*>& positiveDependentActionFluents, std::set<ActionFluent*>& negativeDependentActionFluents);
    void calculateDomain(ActionState const& actions, std::set<double>& res);
    void calculateProbDomain(ActionState const& actions, std::set<double>& res);

    void evaluate(double& res, State const& current, ActionState const& actions);
    void evaluateToPD(DiscretePD& res, State const& current, ActionState const& actions);
    void evaluateToKleene(std::set<double>& res, KleeneState const& current, ActionState const& actions);

    void print(std::ostream& out);
};

class Subtraction : public Connective {
public:
    Subtraction(std::vector<LogicalExpression*>& _exprs) :
        Connective(_exprs) {}

    LogicalExpression* instantiate(UnprocessedPlanningTask* task, std::map<std::string, Object*>& replacements);
    LogicalExpression* replaceQuantifier(UnprocessedPlanningTask* task, std::map<std::string, std::string>& replacements, Instantiator* instantiator);
    LogicalExpression* simplify(UnprocessedPlanningTask* task, std::map<StateFluent*,NumericConstant*>& replacements);

    LogicalExpression* determinizeMostLikely(NumericConstant* randomNumberReplacement);

    void collectInitialInfo(bool& isProbabilistic, bool& containsArithmeticFunction, std::set<StateFluent*>& dependentStateFluents, 
                                    std::set<ActionFluent*>& positiveDependentActionFluents, std::set<ActionFluent*>& negativeDependentActionFluents);
    void calculateDomain(ActionState const& actions, std::set<double>& res);
    void calculateProbDomain(ActionState const& actions, std::set<double>& res);

    void evaluate(double& res, State const& current, ActionState const& actions);
    void evaluateToPD(DiscretePD& res, State const& current, ActionState const& actions);
    void evaluateToKleene(std::set<double>& res, KleeneState const& current, ActionState const& actions);

    void print(std::ostream& out);
};

class Multiplication : public Connective {
public:
    Multiplication(std::vector<LogicalExpression*>& _exprs) :
        Connective(_exprs) {}

    LogicalExpression* instantiate(UnprocessedPlanningTask* task, std::map<std::string, Object*>& replacements);
    LogicalExpression* replaceQuantifier(UnprocessedPlanningTask* task, std::map<std::string, std::string>& replacements, Instantiator* instantiator);
    LogicalExpression* simplify(UnprocessedPlanningTask* task, std::map<StateFluent*,NumericConstant*>& replacements);

    LogicalExpression* determinizeMostLikely(NumericConstant* randomNumberReplacement);

    void collectInitialInfo(bool& isProbabilistic, bool& containsArithmeticFunction, std::set<StateFluent*>& dependentStateFluents, 
                                    std::set<ActionFluent*>& positiveDependentActionFluents, std::set<ActionFluent*>& negativeDependentActionFluents);
    void calculateDomain(ActionState const& actions, std::set<double>& res);
    void calculateProbDomain(ActionState const& actions, std::set<double>& res);

    void evaluate(double& res, State const& current, ActionState const& actions);
    void evaluateToPD(DiscretePD& res, State const& current, ActionState const& actions);
    void evaluateToKleene(std::set<double>& res, KleeneState const& current, ActionState const& actions);

    void print(std::ostream& out);
};

class Division : public Connective {
public:
    Division(std::vector<LogicalExpression*>& _exprs) :
        Connective(_exprs) {}

    LogicalExpression* instantiate(UnprocessedPlanningTask* task, std::map<std::string, Object*>& replacements);
    LogicalExpression* replaceQuantifier(UnprocessedPlanningTask* task, std::map<std::string, std::string>& replacements, Instantiator* instantiator);
    LogicalExpression* simplify(UnprocessedPlanningTask* task, std::map<StateFluent*,NumericConstant*>& replacements);

    LogicalExpression* determinizeMostLikely(NumericConstant* randomNumberReplacement);

    void collectInitialInfo(bool& isProbabilistic, bool& containsArithmeticFunction, std::set<StateFluent*>& dependentStateFluents, 
                                    std::set<ActionFluent*>& positiveDependentActionFluents, std::set<ActionFluent*>& negativeDependentActionFluents);
    void calculateDomain(ActionState const& actions, std::set<double>& res);
    void calculateProbDomain(ActionState const& actions, std::set<double>& res);

    void evaluate(double& res, State const& current, ActionState const& actions);
    void evaluateToPD(DiscretePD& res, State const& current, ActionState const& actions);
    void evaluateToKleene(std::set<double>& res, KleeneState const& current, ActionState const& actions);

    void print(std::ostream& out);
};

/*****************************************************************
                          Unaries
*****************************************************************/

class NegateExpression : public LogicalExpression {
public:
    LogicalExpression* expr;

    NegateExpression(LogicalExpression* _expr) :
        expr(_expr) {}

    LogicalExpression* replaceQuantifier(UnprocessedPlanningTask* task, std::map<std::string, std::string>& replacements, Instantiator* instantiator);
    LogicalExpression* instantiate(UnprocessedPlanningTask* task, std::map<std::string, Object*>& replacements);
    LogicalExpression* simplify(UnprocessedPlanningTask* task, std::map<StateFluent*,NumericConstant*>& replacements);

    LogicalExpression* determinizeMostLikely(NumericConstant* randomNumberReplacement);

    void collectInitialInfo(bool& isProbabilistic, bool& containsArithmeticFunction, std::set<StateFluent*>& dependentStateFluents, 
                                    std::set<ActionFluent*>& positiveDependentActionFluents, std::set<ActionFluent*>& negativeDependentActionFluents);
    void calculateDomain(ActionState const& actions, std::set<double>& res);
    void calculateProbDomain(ActionState const& actions, std::set<double>& res);

    void evaluate(double& res, State const& current, ActionState const& actions);
    void evaluateToPD(DiscretePD& res, State const& current, ActionState const& actions);
    void evaluateToKleene(std::set<double>& res, KleeneState const& current, ActionState const& actions);

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

    LogicalExpression* instantiate(UnprocessedPlanningTask* task, std::map<std::string, Object*>& replacements);
    LogicalExpression* replaceQuantifier(UnprocessedPlanningTask* task, std::map<std::string, std::string>& replacements, Instantiator* instantiator);
    LogicalExpression* simplify(UnprocessedPlanningTask* task, std::map<StateFluent*,NumericConstant*>& replacements);

    void print(std::ostream& out);

    LogicalExpression* expr;
};

class BernoulliDistribution : public LogicalExpression {
public:
    BernoulliDistribution(LogicalExpression* _expr) :
        LogicalExpression(),
        expr(_expr) {}

    LogicalExpression* instantiate(UnprocessedPlanningTask* task, std::map<std::string, Object*>& replacements);
    LogicalExpression* replaceQuantifier(UnprocessedPlanningTask* task, std::map<std::string, std::string>& replacements, Instantiator* instantiator);
    LogicalExpression* simplify(UnprocessedPlanningTask* task, std::map<StateFluent*,NumericConstant*>& replacements);

    LogicalExpression* determinizeMostLikely(NumericConstant* randomNumberReplacement);

    void collectInitialInfo(bool& isProbabilistic, bool& containsArithmeticFunction, std::set<StateFluent*>& dependentStateFluents, 
                                    std::set<ActionFluent*>& positiveDependentActionFluents, std::set<ActionFluent*>& negativeDependentActionFluents);
    void calculateDomain(ActionState const& actions, std::set<double>& res);
    void calculateProbDomain(ActionState const& actions, std::set<double>& res);

    void evaluate(double& res, State const& current, ActionState const& actions);
    void evaluateToPD(DiscretePD& res, State const& current, ActionState const& actions);
    void evaluateToKleene(std::set<double>& res, KleeneState const& current, ActionState const& actions);

    void print(std::ostream& out);

    LogicalExpression* expr;
};

class DiscreteDistribution : public LogicalExpression {
public:
    std::vector<LogicalExpression*> values;
    std::vector<LogicalExpression*> probabilities;
    
    DiscreteDistribution(std::vector<LogicalExpression*> _values, std::vector<LogicalExpression*> _probabilities) :
    	LogicalExpression(), values(_values), probabilities(_probabilities) {}

    LogicalExpression* replaceQuantifier(UnprocessedPlanningTask* task, std::map<std::string, std::string>& replacements, Instantiator* instantiator);
    LogicalExpression* instantiate(UnprocessedPlanningTask* task, std::map<std::string, Object*>& replacements);
    LogicalExpression* simplify(UnprocessedPlanningTask* task, std::map<StateFluent*,NumericConstant*>& replacements);

    LogicalExpression* determinizeMostLikely(NumericConstant* randomNumberReplacement);

    void collectInitialInfo(bool& isProbabilistic, bool& containsArithmeticFunction, std::set<StateFluent*>& dependentStateFluents, 
                            std::set<ActionFluent*>& positiveDependentActionFluents, std::set<ActionFluent*>& negativeDependentActionFluents);
    void calculateDomain(ActionState const& actions, std::set<double>& res);
    void calculateProbDomain(ActionState const& actions, std::set<double>& res);

    void evaluate(double& res, State const& current, ActionState const& actions);
    void evaluateToPD(DiscretePD& res, State const& current, ActionState const& actions);
    void evaluateToKleene(std::set<double>& res, KleeneState const& current, ActionState const& actions);

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

    LogicalExpression* replaceQuantifier(UnprocessedPlanningTask* task, std::map<std::string, std::string>& replacements, Instantiator* instantiator);
    LogicalExpression* instantiate(UnprocessedPlanningTask* task, std::map<std::string, Object*>& replacements);
    LogicalExpression* simplify(UnprocessedPlanningTask* task, std::map<StateFluent*,NumericConstant*>& replacements);

    LogicalExpression* determinizeMostLikely(NumericConstant* randomNumberReplacement);

    void collectInitialInfo(bool& isProbabilistic, bool& containsArithmeticFunction, std::set<StateFluent*>& dependentStateFluents, 
                                    std::set<ActionFluent*>& positiveDependentActionFluents, std::set<ActionFluent*>& negativeDependentActionFluents);
    void calculateDomain(ActionState const& actions, std::set<double>& res);
    void calculateProbDomain(ActionState const& actions, std::set<double>& res);

    void evaluate(double& res, State const& current, ActionState const& actions);
    void evaluateToPD(DiscretePD& res, State const& current, ActionState const& actions);
    void evaluateToKleene(std::set<double>& res, KleeneState const& current, ActionState const& actions);

    void print(std::ostream& out);
};

class MultiConditionChecker : public LogicalExpression {
public:
    std::vector<LogicalExpression*> conditions;
    std::vector<LogicalExpression*> effects;
    bool hasUncertainCondition;

    MultiConditionChecker(std::vector<LogicalExpression*> _conditions, std::vector<LogicalExpression*> _effects) :
        conditions(_conditions), effects(_effects), hasUncertainCondition(false) {}

    LogicalExpression* replaceQuantifier(UnprocessedPlanningTask* task, std::map<std::string, std::string>& replacements, Instantiator* instantiator);
    LogicalExpression* instantiate(UnprocessedPlanningTask* task, std::map<std::string, Object*>& replacements);
    LogicalExpression* simplify(UnprocessedPlanningTask* task, std::map<StateFluent*,NumericConstant*>& replacements);

    LogicalExpression* determinizeMostLikely(NumericConstant* randomNumberReplacement);

    void collectInitialInfo(bool& isProbabilistic, bool& containsArithmeticFunction, std::set<StateFluent*>& dependentStateFluents, 
                                    std::set<ActionFluent*>& positiveDependentActionFluents, std::set<ActionFluent*>& negativeDependentActionFluents);
    void calculateDomain(ActionState const& actions, std::set<double>& res);
    void calculateProbDomain(ActionState const& actions, std::set<double>& res);

    void evaluate(double& res, State const& current, ActionState const& actions);
    void evaluateToPD(DiscretePD& res, State const& current, ActionState const& actions);
    void evaluateToKleene(std::set<double>& res, KleeneState const& current, ActionState const& actions);

    void print(std::ostream& out);
};

#endif
