#ifndef PLANNING_TASK_H
#define PLANNING_TASK_H

#include <map>

#include "states.h"

class Type;
class Parameter;
class Object;
class ParametrizedVariable;
class LogicalExpression;
class NonFluent;
class StateFluent;
class RewardFunction;

struct PlanningTask {
    PlanningTask();

    void print(std::ostream& out);

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

    // This instance's name
    std::string name;

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
    std::vector<StateFluent*> stateFluents;
    std::vector<ActionFluent*> actionFluents;
    std::vector<NonFluent*> nonFluents;
    std::map<ParametrizedVariable*, std::vector<StateFluent*> > variablesBySchema;

    // State action constraints
    std::vector<LogicalExpression*> SACs;
    std::vector<ActionPrecondition*> dynamicSACs;
    std::vector<ActionPrecondition*> staticSACs;
    std::vector<ActionPrecondition*> stateInvariants;

    // Instantiated CPFs
    std::vector<ConditionalProbabilityFunction*> CPFs;
    RewardFunction* rewardCPF;

    // Legal action states
    std::vector<ActionState> actionStates;

    // (Non-trivial) properties
    bool rewardFormulaAllowsRewardLockDetection;
    bool rewardLockDetected;
    std::string finalRewardCalculationMethod;
    std::vector<int> candidatesForOptimalFinalAction;
    bool unreasonableActionDetected;
    bool unreasonableActionInDeterminizationDetected;
    int nonTerminalStatesWithUniqueAction;
    int numberOfEncounteredStates;

    // Hash Keys
    std::vector<std::vector<long> > stateHashKeys;
    std::vector<long> kleeneStateHashKeyBases;

    std::vector<std::vector<std::pair<int,long> > > indexToStateFluentHashKeyMap;
    std::vector<std::vector<std::pair<int,long> > > indexToKleeneStateFluentHashKeyMap;

    // Random training set of reachable states
    std::set<State, State::StateSort> trainingSet;
};

#endif

