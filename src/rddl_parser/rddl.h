#ifndef RDDL_H
#define RDDL_H

#include <set>
#include <vector>
#include <string>
#include <map>

class LogicalExpression;
class PlanningTask;

class PvarDefinition;
class PvarExpression;
class Object;
class Type;
class ParametrizedVariable;

/*****************************************************************
                            Non Fluents
******************************************************************/

class PvariablesInstanceDefine {
public:
    PvariablesInstanceDefine(std::string _name, double _value,
                             std::vector<std::string>* _lConstList = nullptr)
        : name(_name), initValue(_value), lConstList(_lConstList) {}
    ~PvariablesInstanceDefine() {
        delete lConstList;
    }

    std::string getName() const {
        return name;
    }
    std::vector<std::string>* getLConstList() const {
        return lConstList;
    }
    double getInitValue() const {
        return initValue;
    }

private:
    std::string name;
    double initValue;
    std::vector<std::string>* lConstList;
};


class ObjectDefine {
public:
    ObjectDefine(std::string _typeName, std::vector<std::string>* _objectNames)
        :typeName(_typeName), objectNames(_objectNames) {}
    ~ObjectDefine() {
        delete objectNames;
    }

    std::string getTypeName() const {
         return typeName;
    }
    std::vector<std::string>* getObjectNames() const {
        return objectNames;
    }

private:
    std::string typeName;
    std::vector<std::string>* objectNames;
};


class NonFluentBlock {
public:
    NonFluentBlock(std::string _name, std::string _domainName, std::vector<PvariablesInstanceDefine*>* _nonFluents, std::vector<ObjectDefine*>* _objects = nullptr)
        :name(_name), domainName(_domainName), nonFluents(_nonFluents), objects(_objects) {}

    ~NonFluentBlock() {
        delete objects;
        delete nonFluents;
    }

    std::string getName() const {
        return name;
    }
    std::string getDomainName() const {
        return domainName;
    }
    std::vector<ObjectDefine*>* getObjects() const {
        return objects;
    }
    std::vector<PvariablesInstanceDefine*>* getNonFluents() const {
        return nonFluents;
    }

private:
    std::string name;
    std::string domainName;
    std::vector<PvariablesInstanceDefine*>* nonFluents;
    std::vector<ObjectDefine*>* objects;
};

/*****************************************************************
                            Domain
******************************************************************/

static const std::set<std::string> validRequirements = {"continuous", "multivalued", "reward-deterministic", "intermediate-nodes", "constrained-state", "partially-observed", "concurrent", "integer-valued", "cpf-deterministic" };

class CaseDefine {
public:
    CaseDefine(LogicalExpression* _condition, LogicalExpression* _effect)
        :condition(_condition), effect(_effect) {}
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
    LConstCaseList()
        :values(new std::vector<LogicalExpression*>()), probabilites(new std::vector<LogicalExpression*>()) {}
    ~LConstCaseList() {
        delete values;
        delete probabilites;
    }

    void addValue(LogicalExpression* expr) {
        values->push_back(expr);
    }
    void addProbability(LogicalExpression* expr) {
        probabilites->push_back(expr);
    }
    std::vector<LogicalExpression*>* getValues() const {
        return values;
    }
    std::vector<LogicalExpression*>* getProbabilities() const {
        return probabilites;
    }

private:
    std::vector<LogicalExpression*>* values;
    std::vector<LogicalExpression*>* probabilites;
};

class PvarExpression {
public:
    PvarExpression(std::string _name, std::vector<std::string>* _parameters = nullptr)
        :name(_name), parameters(_parameters) {}
    ~PvarExpression() {
        delete parameters;
    }

    std::string getName() const {
        return name;
    }
    std::vector<std::string>* getParameters() const {
        return parameters;
    }

private:
    std::string name;
    std::vector<std::string>* parameters;
};

class CpfDefinition {
public:
    CpfDefinition(PvarExpression* _pVarExpression, LogicalExpression* _logicalExpression)
        :pVarExpression(_pVarExpression), logicalExpression(_logicalExpression) {}
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
    PvarDefinition(std::string _name, std::vector<std::string>* _parameteres,
                   std::string _varType, std::string _defaultValueType,
                   std::string _satisfactionType = "", std::string _defaultVarValue = "")
        :name(_name), parameters(_parameteres), varType(_varType), defaultVarType(_defaultValueType), satisfactionType(_satisfactionType), defaultVarValue(_defaultVarValue) {}
    ~PvarDefinition() {
        delete parameters;
    }

    std::string getName() const {
        return name;
    }
    std::vector<std::string>* getParameters() const {
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
    std::vector<std::string>* parameters;
    std::string varType;
    std::string defaultVarType;
    std::string satisfactionType;
    std::string defaultVarValue;
};


class DefineType {
public:
    DefineType(std::string _name, std::string _superType)
        :name(_name), superType(_superType) {
        superTypeList = NULL;
    }
    DefineType(std::string _name, std::vector<std::string>* _superTypeList)
        :name(_name), superTypeList(_superTypeList) {}
    ~DefineType() {
        delete superTypeList;
    }

    std::string getName() const {
        return name;
    }
    std::string getSuperType() const {
        return superType;
    }
    std::vector<std::string>* getSuperTypeList() const {
        return superTypeList;
    }

private:
    std::string name;
    std::string superType;
    std::vector<std::string>* superTypeList;
};

class DomainList {
public:
    DomainList() {}
    ~DomainList() {}

    std::vector<DefineType*>* getTypes() const {
        return types;
    }
    std::vector<PvarDefinition*>* getPVars() const {
        return pVariables;
    }
    std::vector<CpfDefinition*>* getCpfs() const {
        return cpfs;
    }
    LogicalExpression* getReward() const {
        return reward;
    }
    std::vector<ObjectDefine*>* getObjects() const {
        return objects;
    }
    std::vector<LogicalExpression*>* getStateConstraints() const {
        return stateConstraints;
    }

    void setTypes(std::vector<DefineType*>* _types) {
        types = _types;
    }
    void setPvar(std::vector<PvarDefinition*>* _pVariables) {
        pVariables = _pVariables;
    }
    void setCPF(std::vector<CpfDefinition*>* _cpfs) {
        cpfs = _cpfs;
    }
    void setReward(LogicalExpression* _reward) {
        reward = _reward;
    }
    void setStateConstraint(std::vector<LogicalExpression*>* _stateConstraints) {
        stateConstraints = _stateConstraints;
    }
    void setObjects(std::vector<ObjectDefine*>* _objects) {
        objects = _objects;
    }

private:
    std::vector<DefineType*>* types;
    std::vector<PvarDefinition*>* pVariables;
    std::vector<CpfDefinition*>* cpfs;
    LogicalExpression* reward;
    std::vector<LogicalExpression*>* stateConstraints;
    std::vector<ObjectDefine*>* objects;
};

class Domain {
public:
    Domain(std::string* _name, std::vector<std::string>* _requirements, DomainList* _domainList)
        :name(_name), requirements(_requirements), domainList(_domainList) {}
    Domain(std::string* _name, DomainList* _domainList)
        :name(_name), domainList(_domainList) {}
    ~Domain() {
        delete requirements;
        delete domainList;
        delete name;
    }

    static std::string validRequirement(std::string req);

    std::vector<ObjectDefine*>* getObjects() const {
        return domainList->getObjects();
    }
    std::vector<DefineType*>* getDomainTypes() const {
        return domainList->getTypes();
    }
    std::vector<PvarDefinition*>* getPvarDefinitions() const {
        return domainList->getPVars();
    }
    std::vector<CpfDefinition*>* getCpfs() const {
        return domainList->getCpfs();
    }
    LogicalExpression* getReward() const {
        return domainList->getReward();
    }
    std::vector<LogicalExpression*>* getStateConstraints() const {
        return domainList->getStateConstraints();
    }
    std::string getName() const {
        return *name;
    }

private:
    std::string* name;
    std::vector<std::string>* requirements;
    DomainList* domainList;
};

/*****************************************************************
                             Instance
******************************************************************/

class Instance {
public:
    Instance(std::string _name, std::string _domainName, std::string _nonFluentsName, std::vector<PvariablesInstanceDefine*>* _pVariables, int _maxNonDefAction, int _horizon, double _discount)
        :name(_name), domainName(_domainName), nonFluentsName(_nonFluentsName), pVariables(_pVariables), maxNonDefActions(_maxNonDefAction), horizon(_horizon), discount(_discount) {}
    ~Instance() {
        delete pVariables;
    }

    std::string getName() const {
        return name;
    }
    std::string getDomainName() const {
        return domainName;
    }
    std::string getNonFluentsName() const {
        return nonFluentsName;
    }
    std::vector<PvariablesInstanceDefine*>* getPVariables() const {
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
    std::vector<PvariablesInstanceDefine*>* pVariables;
    int maxNonDefActions;
    int horizon;
    double discount;
};

/*****************************************************************
                           RDDL Block
*****************************************************************/

class RDDLBlock {
public:
    RDDLBlock();
    ~RDDLBlock() {}

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

    PlanningTask* task;
private:
    std::string domainName;
    std::string nonFluentsName;
};

/*****************************************************************
                           Helper Methods
*****************************************************************/
ParametrizedVariable* getParametrizedVariableFromPvarDefinition(std::string name);
void storeParametrizedVariableFromPvarDefinition(std::string pVarName, PvarDefinition* pVarDefinition);
void storeParametrizedVariableMap(std::string pVarName, PvarExpression* pVarExpression);
bool storeObject(std::string objName, std::string objectType);
Object* getObject(std::string objName);
bool storeType(std::string typeName, std::string superTypeName);
Type* getType(std::string typeName);

#endif
