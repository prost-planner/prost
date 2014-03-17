#ifndef UNPROCESSED_PLANNING_TASK_H
#define UNPROCESSED_PLANNING_TASK_H

#include <string>
#include <vector>
#include <map>
#include <cassert>

class Type;
class Parameter;
class Object;
class ParametrizedVariable;
class LogicalExpression;
class NonFluent;
class StateFluent;
class ActionFluent;
class ActionState;
class Evaluatable;
class ConditionalProbabilityFunction;
class RewardFunction;

struct UnprocessedPlanningTask {
    UnprocessedPlanningTask(std::string _domainDesc, std::string _problemDesc);

    void preprocessInput(std::string& problemDesc);

    void addType(std::string const& name, std::string const& superType = "");
    void addObject(std::string const& typeName, std::string const& objectName);

    void addVariableDefinition(ParametrizedVariable* varDef);

    void addParametrizedVariable(ParametrizedVariable* parent, std::vector<Parameter*> const& params);
    void addParametrizedVariable(ParametrizedVariable* parent, std::vector<Parameter*> const& params, double initialValue);

    StateFluent* getStateFluent(std::string const& name);
    ActionFluent* getActionFluent(std::string const& name);
    NonFluent* getNonFluent(std::string const& name);

    std::vector<StateFluent*> getVariablesOfSchema(ParametrizedVariable* schema);

    void addStateActionConstraint(LogicalExpression* sac);
    void addCPF(ConditionalProbabilityFunction* const& cpf);
    void setRewardCPF(LogicalExpression* const& rewardFormula);

    // Task description as string
    std::string domainDesc;
    std::string nonFluentsDesc;
    std::string instanceDesc;

    // Names
    std::string domainName;
    std::string nonFluentsName;
    std::string instanceName;

    // (Trivial) properties
    int numberOfConcurrentActions;
    int horizon;
    double discountFactor;

    // Object types
    std::map<std::string, Type*> types;
    std::map<std::string, Object*> objects;

    // Schematic variables and CPFs
    std::map<std::string, ParametrizedVariable*> variableDefinitions;
    std::map<ParametrizedVariable*, LogicalExpression*> CPFDefinitions;

    // Instantiated variables
    std::map<std::string, StateFluent*> stateFluents;
    std::map<std::string, ActionFluent*> actionFluents;
    std::map<std::string, NonFluent*> nonFluents;
    std::map<ParametrizedVariable*, std::vector<StateFluent*> > variablesBySchema;

    // State action constraints
    std::vector<LogicalExpression*> SACs;
    std::vector<Evaluatable*> dynamicSACs;
    std::vector<Evaluatable*> staticSACs;
    std::vector<Evaluatable*> stateInvariants;

    // Instantiated CPFs
    std::vector<ConditionalProbabilityFunction*> CPFs;
    RewardFunction* rewardCPF;

    // Legal action states
    std::vector<ActionState> actionStates;

    // (Non-trivial) properties
    bool noopIsOptimalFinalAction;
    bool rewardFormulaAllowsRewardLockDetection;

    // Hash Keys
    std::vector<std::vector<long> > stateHashKeys;
    std::vector<long> kleeneStateHashKeyBases;

    std::vector<std::vector<std::pair<int,long> > > indexToStateFluentHashKeyMap;
    std::vector<std::vector<std::pair<int,long> > > indexToKleeneStateFluentHashKeyMap;
};

#endif

