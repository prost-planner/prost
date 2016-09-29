#ifndef RDDL_H
#define RDDL_H

#include <map>
#include <set>
#include <string>
#include <vector>

#include "states.h"

class Type;
class Parameter;
class Object;
class ParametrizedVariable;
class LogicalExpression;
class NonFluent;
class StateFluent;
class RewardFunction;

class VariableSchematic;
class VariableExpression;
class RDDLTask;

/*****************************************************************
                            Non Fluents
******************************************************************/

class VariableInstanceSchematic {
public:
    VariableInstanceSchematic(std::string _name, double _value,
                             std::vector<std::string> _parameters = {})
        : name(_name), initValue(_value), parameters(_parameters) {}

    std::string getName() const {
        return name;
    }
    std::vector<std::string> const& getParameters() const {
        return parameters;
    }
    double getInitValue() const {
        return initValue;
    }

private:
    std::string name;
    double initValue;
    std::vector<std::string> parameters;
};

class ObjectSchematic {
public:
    ObjectSchematic(std::string _typeName, std::vector<std::string> _objectNames)
        : typeName(_typeName), objectNames(_objectNames) {}

    std::string getTypeName() const {
        return typeName;
    }
    std::vector<std::string> const& getObjectNames() const {
        return objectNames;
    }

private:
    std::string typeName;
    std::vector<std::string> objectNames;
};

class NonFluentBlock {
public:
    NonFluentBlock(std::string _name, std::string _domainName,
                   std::vector<VariableInstanceSchematic*> _nonFluents,
                   std::vector<ObjectSchematic*> _objects = {})
        : name(_name),
          domainName(_domainName),
          nonFluents(_nonFluents),
          objects(_objects) {}

    std::string getName() const {
        return name;
    }
    std::string getDomainName() const {
        return domainName;
    }
    std::vector<ObjectSchematic*> const& getObjects() const {
        return objects;
    }
    std::vector<VariableInstanceSchematic*> const& getNonFluents() const {
        return nonFluents;
    }

private:
    std::string name;
    std::string domainName;
    std::vector<VariableInstanceSchematic*> nonFluents;
    std::vector<ObjectSchematic*> objects;
};

/*****************************************************************
                            Domain
******************************************************************/
class CaseSchematic {
public:
    CaseSchematic(LogicalExpression* _condition, LogicalExpression* _effect)
        : condition(_condition), effect(_effect) {}
    ~CaseSchematic();

    LogicalExpression* getCondition() const {
        return condition;
    }
    LogicalExpression* getEffect() const {
        return effect;
    }

private:
    LogicalExpression* condition;
    LogicalExpression* effect;
};

class LConstCaseList {
public:
    LConstCaseList() = default;

    void addValue(LogicalExpression* expr) {
        values.push_back(expr);
    }
    void addProbability(LogicalExpression* expr) {
        probabilites.push_back(expr);
    }
    std::vector<LogicalExpression*> const& getValues() const {
        return values;
    }
    std::vector<LogicalExpression*> const& getProbabilities() const {
        return probabilites;
    }

private:
    std::vector<LogicalExpression*> values;
    std::vector<LogicalExpression*> probabilites;
};

class VariableExpression {
public:
    VariableExpression(std::string _name, std::vector<std::string> _parameters = {})
        : name(_name), parameters(_parameters) {}

    std::string getName() const {
        return name;
    }
    std::vector<std::string> const& getParameters() const {
        return parameters;
    }

private:
    std::string name;
    std::vector<std::string> parameters;
};

class CPFSchematic {
public:
    CPFSchematic(VariableExpression* _variableExpression,
                  LogicalExpression* _logicalExpression)
        : variableExpression(_variableExpression),
          logicalExpression(_logicalExpression) {}

    VariableExpression* getVariable() const {
        return variableExpression;
    }
    LogicalExpression* getLogicalExpression() const {
        return logicalExpression;
    }

private:
    VariableExpression* variableExpression;
    LogicalExpression* logicalExpression;
};

class VariableSchematic {
public:
    VariableSchematic(std::string _name, std::vector<std::string> _parameters,
                   std::string _varType, std::string _defaultValueType,
                   std::string _satisfactionType = "",
                   std::string _defaultVarValue = "")
        : name(_name),
          parameters(_parameters),
          varType(_varType),
          defaultVarType(_defaultValueType),
          satisfactionType(_satisfactionType),
          defaultVarValue(_defaultVarValue) {}

    std::string getName() const {
        return name;
    }
    std::vector<std::string> const& getParameters() const {
        return parameters;
    }
    std::string getVarType() const {
        return varType;
    }
    std::string getDefaultVarType() const {
        return defaultVarType;
    }
    std::string getSatisfactionType() const {
        return satisfactionType;
    }
    std::string getDefaultVarValue() const {
        return defaultVarValue;
    }

private:
    std::string name;
    std::vector<std::string> parameters;
    std::string varType;
    std::string defaultVarType;
    std::string satisfactionType;
    std::string defaultVarValue;
};

class SchematicType {
public:
    SchematicType(std::string _name, std::string _superType)
        : name(_name), superType(_superType) {}
    SchematicType(std::string _name, std::vector<std::string> _superTypeList)
        : name(_name), superTypeList(_superTypeList) {}

    std::string getName() const {
        return name;
    }
    std::string getSuperType() const {
        return superType;
    }
    std::vector<std::string> const& getSuperTypeList() const {
        return superTypeList;
    }

private:
    std::string name;
    std::string superType;
    std::vector<std::string> superTypeList;
};

class DomainList {
public:
    DomainList() = default;

    std::vector<SchematicType*> const& getTypes() const {
        return types;
    }
    std::vector<VariableSchematic*> const& getVariables() const {
        return variables;
    }
    std::vector<CPFSchematic*> const& getCPFs() const {
        return CPFs;
    }
    LogicalExpression* getReward() const {
        return reward;
    }
    std::vector<ObjectSchematic*> const& getObjects() const {
        return objects;
    }
    std::vector<LogicalExpression*> const& getStateConstraints() const {
        return stateConstraints;
    }

    void setTypes(std::vector<SchematicType*>& _types) {
        types = _types;
    }
    void setVariables(std::vector<VariableSchematic*>& _variables) {
        variables = _variables;
    }
    void setCPF(std::vector<CPFSchematic*>& _CPFs) {
        CPFs = _CPFs;
    }
    void setReward(LogicalExpression* _reward) {
        reward = _reward;
    }
    void setStateConstraint(
        std::vector<LogicalExpression*>& _stateConstraints) {
        stateConstraints = _stateConstraints;
    }
    void setObjects(std::vector<ObjectSchematic*>& _objects) {
        objects = _objects;
    }

private:
    std::vector<SchematicType*> types;
    std::vector<VariableSchematic*> variables;
    std::vector<CPFSchematic*> CPFs;
    LogicalExpression* reward;
    std::vector<LogicalExpression*> stateConstraints;
    std::vector<ObjectSchematic*> objects;
};

class Domain {
public:
    Domain(std::string _name, DomainList _domainList,
           std::vector<std::string> _requirements = {})
        : name(_name), domainList(_domainList), requirements(_requirements) {}

    static std::string validRequirement(RDDLTask rddlTask, std::string req);

    std::vector<ObjectSchematic*> const& getObjects() const {
        return domainList.getObjects();
    }
    std::vector<SchematicType*> const& getDomainTypes() const {
        return domainList.getTypes();
    }
    std::vector<VariableSchematic*> const& getVariableSchematics() const {
        return domainList.getVariables();
    }
    std::vector<CPFSchematic*> const& getCPFs() const {
        return domainList.getCPFs();
    }
    LogicalExpression* getReward() const {
        return domainList.getReward();
    }
    std::vector<LogicalExpression*> const& getStateConstraints() const {
        return domainList.getStateConstraints();
    }
    std::string getName() const {
        return name;
    }

private:
    std::string name;
    DomainList domainList;
    std::vector<std::string> requirements;
};

/*****************************************************************
                             Instance
******************************************************************/

class Instance {
public:
    Instance(std::string _name, std::string _domainName,
             std::string _nonFluentsName,
             std::vector<VariableInstanceSchematic*> _variables,
             int _maxNonDefActions, int _horizon, double _discount)
        : name(_name),
          domainName(_domainName),
          nonFluentsName(_nonFluentsName),
          variables(_variables),
          maxNonDefActions(_maxNonDefActions),
          horizon(_horizon),
          discount(_discount) {}

    std::string getName() const {
        return name;
    }
    std::string getDomainName() const {
        return domainName;
    }
    std::string getNonFluentsName() const {
        return nonFluentsName;
    }
    std::vector<VariableInstanceSchematic*> const& getVariables() const {
        return variables;
    }
    int getMaxNonDefActions() const {
        return maxNonDefActions;
    }
    int getHorizon() const {
        return horizon;
    }
    double getDiscount() const {
        return discount;
    }

private:
    std::string name;
    std::string domainName;
    std::string nonFluentsName;
    std::vector<VariableInstanceSchematic*> variables;
    int maxNonDefActions;
    int horizon;
    double discount;
};

/*****************************************************************
                           RDDL Block
*****************************************************************/

class RDDLTask {
public:
    RDDLTask();
    ~RDDLTask() {}

    void addDomain(Domain* d);
    void addInstance(Instance* i);
    void addNonFluent(NonFluentBlock* nf);

    void execute(std::string targetDir);

    std::string getDomainName() const {
        return domainName;
    }
    std::string getNonFluentsName() const {
        return nonFluentsName;
    }

    // Sub-methods for domain parse
    void addTypes(Domain* domain);
    void addVariables(Domain* domain);
    void addCPFs(Domain* domain);
    void setReward(Domain* domain);
    void addStateConstraints(Domain* domain);
    void addObjects(Domain* domain);

    // Following methods are PlanningTask methods
    void print(std::ostream& out);

    void addType(std::string const& name, std::string const& superType = "");
    void addObject(std::string const& typeName, std::string const& objectName);

    void addVariableSchematic(ParametrizedVariable* varDef);

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

    // The following are PlanningTask variables

    // This instance's name
    // TODO: move this to the private members??
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

    // Requirements section
    std::set<std::string> validRequirements;

    // Helper Methods
    std::map<std::string, VariableSchematic*>
        parametrizedVariableSchematicsMap; // Map for storing definition of
                                            // ParametrizedVariables
    std::map<std::string, VariableExpression*>
        parametrizedVariableMap; // Map for storing ParametrizedVariables as
                                 // expressions

    ParametrizedVariable* getParametrizedVariableFromVariableSchematic(
        std::string name);
    void storeParametrizedVariableFromVariableSchematic(
        std::string varName, VariableSchematic* varSchematic);
    void storeParametrizedVariableMap(std::string varName,
                                      VariableExpression* varExpression);
    Object* getObject(std::string objName);
    Type* getType(std::string typeName);

private:
    std::string domainName;
    std::string nonFluentsName;
};



#endif
