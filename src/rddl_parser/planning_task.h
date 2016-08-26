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

    void addParametrizedVariable(ParametrizedVariable* parent,
                                 std::vector<Parameter*> const& params);
    void addParametrizedVariable(ParametrizedVariable* parent,
                                 std::vector<Parameter*> const& params,
                                 double initialValue);

    StateFluent* getStateFluent(std::string const& name);
    ActionFluent* getActionFluent(std::string const& name);
    NonFluent* getNonFluent(std::string const& name);

    std::vector<StateFluent*> getStateFluentsOfSchema(
        ParametrizedVariable* schema);

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

    // Instantiated variables (Careful with state fluents, they are only used to
    // check for ambiguity. After having read all state fluents, only
    // stateFluentCPFs and their heads are used!)
    std::vector<StateFluent*> stateFluents;
    std::map<std::string, StateFluent*> stateFluentMap;
    std::map<ParametrizedVariable*, std::vector<StateFluent*>>
        stateFluentsBySchema;

    std::vector<ActionFluent*> actionFluents;
    std::map<std::string, ActionFluent*> actionFluentMap;

    std::vector<NonFluent*> nonFluents;
    std::map<std::string, NonFluent*> nonFluentMap;

    // State action constraints
    std::vector<LogicalExpression*> SACs;
    std::vector<ActionPrecondition*> actionPreconds;
    std::vector<ActionPrecondition*> staticSACs;
    std::set<ActionFluent*> primitiveStaticSACs;

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

    // These give the number of (unique) states that were encountered during
    // task analysis. Roughly, the closer these are to each other, the less
    // stochasticity is in the domain.
    int numberOfEncounteredStates;
    int numberOfUniqueEncounteredStates;

    // This is the number of states that was encountered during task analysis.
    // Among other things, it is an indicator for how often the planner can
    // submit the single applicable (reasonable) action and hence save the time.
    int nonTerminalStatesWithUniqueAction;
    int uniqueNonTerminalStatesWithUniqueAction;

    // Hash Keys
    std::vector<std::vector<long>> stateHashKeys;
    std::vector<long> kleeneStateHashKeyBases;

    std::vector<std::vector<std::pair<int, long>>> indexToStateFluentHashKeyMap;
    std::vector<std::vector<std::pair<int, long>>>
        indexToKleeneStateFluentHashKeyMap;

    // Random training set of reachable states
    std::set<State, State::StateSort> trainingSet;
};

#endif
