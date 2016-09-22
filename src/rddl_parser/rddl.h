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

class PvarDefinition;
class PvarExpression;

/*****************************************************************
                            Non Fluents
******************************************************************/

class PvariablesInstanceDefine {
public:
    PvariablesInstanceDefine(std::string _name, double _value,
                             std::vector<std::string> _lConstList = {})
        : name(_name), initValue(_value), lConstList(_lConstList) {}

    std::string getName() const {
        return name;
    }
    std::vector<std::string> const& getLConstList() const {
        return lConstList;
    }
    double getInitValue() const {
        return initValue;
    }

private:
    std::string name;
    double initValue;
    std::vector<std::string> lConstList;
};

class ObjectDefine {
public:
    ObjectDefine(std::string _typeName, std::vector<std::string> _objectNames)
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
                   std::vector<PvariablesInstanceDefine*> _nonFluents,
                   std::vector<ObjectDefine*> _objects = {})
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
    std::vector<ObjectDefine*> const& getObjects() const {
        return objects;
    }
    std::vector<PvariablesInstanceDefine*> const& getNonFluents() const {
        return nonFluents;
    }

private:
    std::string name;
    std::string domainName;
    std::vector<PvariablesInstanceDefine*> nonFluents;
    std::vector<ObjectDefine*> objects;
};

/*****************************************************************
                            Domain
******************************************************************/

static const std::set<std::string> validRequirements = {
    "continuous",         "multivalued",       "reward-deterministic",
    "intermediate-nodes", "constrained-state", "partially-observed",
    "concurrent",         "integer-valued",    "cpf-deterministic"};

class CaseDefine {
public:
    CaseDefine(LogicalExpression* _condition, LogicalExpression* _effect)
        : condition(_condition), effect(_effect) {}
    ~CaseDefine();

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

class PvarExpression {
public:
    PvarExpression(std::string _name, std::vector<std::string> _parameters = {})
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

class CpfDefinition {
public:
    CpfDefinition(PvarExpression* _pVarExpression,
                  LogicalExpression* _logicalExpression)
        : pVarExpression(_pVarExpression),
          logicalExpression(_logicalExpression) {}
    ~CpfDefinition();

    PvarExpression* getPvar() const {
        return pVarExpression;
    }
    LogicalExpression* getLogicalExpression() const {
        return logicalExpression;
    }

private:
    PvarExpression* pVarExpression;
    LogicalExpression* logicalExpression;
};

class PvarDefinition {
public:
    PvarDefinition(std::string _name, std::vector<std::string> _parameters,
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

class DefineType {
public:
    DefineType(std::string _name, std::string _superType)
        : name(_name), superType(_superType) {}
    DefineType(std::string _name, std::vector<std::string> _superTypeList)
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

    std::vector<DefineType*> const& getTypes() const {
        return types;
    }
    std::vector<PvarDefinition*> const& getPVars() const {
        return pVariables;
    }
    std::vector<CpfDefinition*> const& getCpfs() const {
        return cpfs;
    }
    LogicalExpression* getReward() const {
        return reward;
    }
    std::vector<ObjectDefine*> const& getObjects() const {
        return objects;
    }
    std::vector<LogicalExpression*> const& getStateConstraints() const {
        return stateConstraints;
    }

    void setTypes(std::vector<DefineType*>& _types) {
        types = _types;
    }
    void setPvar(std::vector<PvarDefinition*>& _pVariables) {
        pVariables = _pVariables;
    }
    void setCPF(std::vector<CpfDefinition*>& _cpfs) {
        cpfs = _cpfs;
    }
    void setReward(LogicalExpression* _reward) {
        reward = _reward;
    }
    void setStateConstraint(
        std::vector<LogicalExpression*>& _stateConstraints) {
        stateConstraints = _stateConstraints;
    }
    void setObjects(std::vector<ObjectDefine*>& _objects) {
        objects = _objects;
    }

private:
    std::vector<DefineType*> types;
    std::vector<PvarDefinition*> pVariables;
    std::vector<CpfDefinition*> cpfs;
    LogicalExpression* reward;
    std::vector<LogicalExpression*> stateConstraints;
    std::vector<ObjectDefine*> objects;
};

class Domain {
public:
    Domain(std::string _name, DomainList _domainList,
           std::vector<std::string> _requirements = {})
        : name(_name), domainList(_domainList), requirements(_requirements) {}

    static std::string validRequirement(std::string req);

    std::vector<ObjectDefine*> const& getObjects() const {
        return domainList.getObjects();
    }
    std::vector<DefineType*> const& getDomainTypes() const {
        return domainList.getTypes();
    }
    std::vector<PvarDefinition*> const& getPvarDefinitions() const {
        return domainList.getPVars();
    }
    std::vector<CpfDefinition*> const& getCpfs() const {
        return domainList.getCpfs();
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
             std::vector<PvariablesInstanceDefine*> _pVariables,
             int _maxNonDefActions, int _horizon, double _discount)
        : name(_name),
          domainName(_domainName),
          nonFluentsName(_nonFluentsName),
          pVariables(_pVariables),
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
    std::vector<PvariablesInstanceDefine*> const& getPVariables() const {
        return pVariables;
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
    std::vector<PvariablesInstanceDefine*> pVariables;
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
    void addPVars(Domain* domain);
    void addCpfs(Domain* domain);
    void addReward(Domain* domain);
    void addStateConstraints(Domain* domain);
    void addObjects(Domain* domain);

    // Following methods are PlanningTask methods
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

    // The following are PlanningTask variables

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

private:
    std::string domainName;
    std::string nonFluentsName;
};

/*****************************************************************
                           Helper Methods
*****************************************************************/
ParametrizedVariable* getParametrizedVariableFromPvarDefinition(
    std::string name);
void storeParametrizedVariableFromPvarDefinition(
    std::string pVarName, PvarDefinition* pVarDefinition);
void storeParametrizedVariableMap(std::string pVarName,
                                  PvarExpression* pVarExpression);
bool storeObject(std::string objName, std::string objectType);
Object* getObject(std::string objName);
bool storeType(std::string typeName, std::string superTypeName);
Type* getType(std::string typeName);

#endif