/**
    domain.h: classes that are used for definition of domain section in rddl file

    @author Đorđe Relić <dorde.relic@unibas.ch>
    @version 1.0 06/2016
*/

#ifndef DOMAIN_H
#define DOMAIN_H
#include <string>
#include <vector>
#include <set>
#include <iostream>
#include "non_fluents.h"

#include "logical_expressions.h"

static const std::set<std::string> validRequirements = {"continuous", "multivalued", "reward-deterministic", "intermediate-nodes", "constrained-state", "partially-observed", "concurrent", "integer-valued", "cpf-deterministic" };

class CaseDefine {
public:
    CaseDefine(LogicalExpression *_condition, LogicalExpression *_effect)
        :condition(_condition), effect(_effect) {}
    ~CaseDefine();
    LogicalExpression* getCondition();
    LogicalExpression* getEffect();
private:
    LogicalExpression *condition, *effect;
};

class LConstCaseList {
public:
    LConstCaseList()
        :values(new std::vector<LogicalExpression*>()), probabilites(new std::vector<LogicalExpression*>()) {}
    ~LConstCaseList();
    void addValue(LogicalExpression *expr);
    void addProbability(LogicalExpression *expr);
    std::vector<LogicalExpression*>* getValues();
    std::vector<LogicalExpression*>* getProbabilities();
private:
    std::vector<LogicalExpression*> *values;
    std::vector<LogicalExpression*> *probabilites;
};

class PvarExpression {
public:
    PvarExpression(std::string _name, std::vector<std::string> *_parameters = nullptr)
        :name(_name), parameters(_parameters) {}
    ~PvarExpression();
    std::string getName();
    std::vector<std::string>* getParameters();

private:
    std::string name;
    std::vector<std::string> *parameters;
};

class CpfDefinition {
public:
    CpfDefinition(PvarExpression *_pVarExpression, LogicalExpression* _logicalExpression)
        :pVarExpression(_pVarExpression), logicalExpression(_logicalExpression) {}
    ~CpfDefinition();
    PvarExpression* getPvar();
    LogicalExpression* getLogicalExpression();

private:
    PvarExpression *pVarExpression;
    LogicalExpression *logicalExpression;
};

class PvarDefinition {
public:
    PvarDefinition(std::string _name, std::vector<std::string> *_parameteres, std::string _varType, std::string _defaultValueType, std::string _satisfactionType = "", std::string _defaultVarValue = "")
        :name(_name), parameters(_parameteres), varType(_varType), defaultVarType(_defaultValueType), satisfactionType(_satisfactionType), defaultVarValue(_defaultVarValue)
    {}
    ~PvarDefinition();

    std::string getName();
    std::vector<std::string>* getParameters();
    std::string getVarType();
    std::string getDefaultVarType();
    std::string getSatisfactionType();
    std::string getDefaultVarValue();
private:
    std::string name;
    std::vector<std::string> *parameters;
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

    DefineType(std::string _name, std::vector<std::string> *_superTypeList)
        :name(_name), superTypeList(_superTypeList) {}

    ~DefineType();

    std::string getName();
    std::string getSuperType();
    std::vector<std::string>* getSuperTypeList();

private:
    std::string name;
    std::string superType;
    std::vector<std::string> *superTypeList;
};

class DomainList {
public:
    DomainList() {}
    ~DomainList() {}

    void addTypes(std::vector<DefineType*> *_types);
    void addPvar(std::vector<PvarDefinition*> *_pVariables);
    void addCPF(std::vector<CpfDefinition*> *_cpfs);
    void addReward(LogicalExpression *_reward);
    void addStateConstraint(std::vector<LogicalExpression*> *_stateConstraints);
    void addObjects(std::vector<ObjectDefine*> *_objects);
    // void addActionPrecondition() {}
    // void addStateInvariant() {}


    std::vector<DefineType*>* getTypes();
    std::vector<PvarDefinition*>* getPVars();
    std::vector<CpfDefinition*>* getCpfs();
    LogicalExpression* getReward();
    std::vector<LogicalExpression*>* getStateConstraints();
    std::vector<ObjectDefine*>* getObjects();
private:
    std::vector<DefineType*> *types;
    std::vector<PvarDefinition*> *pVariables;
    std::vector<CpfDefinition*> *cpfs;
    LogicalExpression *reward;
    std::vector<LogicalExpression*> *stateConstraints;
    std::vector<ObjectDefine*> *objects;
};

class Domain {
public:
    Domain(std::string *_name, std::vector<std::string> *_requirements, DomainList *_domainList)
        :name(_name), requirements(_requirements), domainList(_domainList) {}

    Domain(std::string *_name, DomainList *_domainList)
        :name(_name), domainList(_domainList) {}

    ~Domain();
    static std::string validRequirement(std::string req);
    std::vector<DefineType*>* getDomainTypes();
    std::vector<PvarDefinition*>* getPvarDefinitions();
    std::vector<CpfDefinition*>* getCpfs();
    LogicalExpression* getReward();
    std::vector<LogicalExpression*>* getStateConstraints();
    std::string getName();
    std::vector<ObjectDefine*>* getObjects();

private:
    std::string *name;
    std::vector<std::string> *requirements;
    DomainList *domainList;
};

#endif