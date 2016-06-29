/**
    domain.cc: implementation of methods from domain.h

    @author Đorđe Relić <dorde.relic@unibas.ch>
    @version 1.0 06/2016
*/

#include "domain.h"

// CaseDefine
LogicalExpression* CaseDefine::getCondition() {
    return condition;
}

LogicalExpression* CaseDefine::getEffect() {
    return effect;
}

// LConstCaseList
LConstCaseList::~LConstCaseList() {
    delete values;
    delete probabilites;
}

void LConstCaseList::addValue(LogicalExpression *expr) {
    values->push_back(expr);
}

void LConstCaseList::addProbability(LogicalExpression *expr) {
    probabilites->push_back(expr);
}

std::vector<LogicalExpression*>* LConstCaseList::getValues() {
    return values;
}

std::vector<LogicalExpression*>* LConstCaseList::getProbabilities() {
    return probabilites;
}

// PvarExpression
PvarExpression::~PvarExpression() {
    delete parameters;
}

std::string PvarExpression::getName() {
    return name;
}

std::vector<std::string>* PvarExpression::getParameters() {
    return parameters;
}

// CpfDefinition
CpfDefinition::~CpfDefinition() {
    delete pVarExpression;
    delete logicalExpression;
}

PvarExpression* CpfDefinition::getPvar() {
    return pVarExpression;
}

LogicalExpression* CpfDefinition::getLogicalExpression() {
    return logicalExpression;
}

// PvarDefinition
PvarDefinition::~PvarDefinition() {
    delete parameters;
}

std::string PvarDefinition::getName() {
    return name;
}

std::vector<std::string>* PvarDefinition::getParameters() {
    return parameters;
}

std::string PvarDefinition::getVarType() {
    return varType;
}

std::string PvarDefinition::getDefaultVarType() {
    return defaultVarType;
}

std::string PvarDefinition::getSatisfactionType() {
    return satisfactionType;
}

std::string PvarDefinition::getDefaultVarValue() {
    return defaultVarValue;
}

// DefineType
DefineType::~DefineType() {
    delete superTypeList;
}

std::string DefineType::getName() {
    return name;
}

std::string DefineType::getSuperType() {
    return superType;
}

std::vector<std::string>* DefineType::getSuperTypeList() {
    return superTypeList;
}

// DomainList
std::vector<DefineType*>* DomainList::getTypes() {
    return types;
}

std::vector<PvarDefinition*>* DomainList::getPVars() {
    return pVariables;
}

std::vector<CpfDefinition*>* DomainList::getCpfs(){
    return cpfs;
}

LogicalExpression* DomainList::getReward() {
    return reward;
}

std::vector<ObjectDefine*>* DomainList::getObjects() {
    return objects;
}

std::vector<LogicalExpression*>* DomainList::getStateConstraints() {
    return stateConstraints;
}

void DomainList::addTypes(std::vector<DefineType*> *_types) {
    types = _types;
}

void DomainList::addPvar(std::vector<PvarDefinition*> *_pVariables) {
    pVariables = _pVariables;
    // std::cout << "Adding variables..." << std::endl;
    // // std::string _name, std::vector<std::string> *_parameteres, std::string _varType, std::string _defaultValueType, double _defaultVarValue
    // for(std::vector<PvarDefinition*>::iterator it = pVariables->begin(); it != pVariables->end(); it++)
    // {
    //     std::cout << "Variable: " << (*it)->getName() << "; varType: " << (*it)->getVarType() << "; defaultVarType: " << (*it)->getDefaultVarType();
    //     std::cout << "; defaultVarValue: " << (*it)->getDefaultVarValue() << std::endl;
    //     std::cout << "With parameters: ";
    //     for(std::vector<std::string>::iterator jt = (*it)->getParameters()->begin(); jt != (*it)->getParameters()->end(); jt++)
    //         std::cout << (*jt) << ", ";
    //     std::cout << std::endl;
    // }
}

void DomainList::addCPF(std::vector<CpfDefinition*> *_cpfs) {
    cpfs = _cpfs;
}

void DomainList::addReward(LogicalExpression *_reward) {
    reward = _reward;
}

void DomainList::addStateConstraint(std::vector<LogicalExpression*> *_stateConstraints) {
    stateConstraints = _stateConstraints;
}

void DomainList::addObjects(std::vector<ObjectDefine*> *_objects) {
    objects = _objects;
}


// Domain
Domain::~Domain() {
    delete requirements;
    delete domainList;
    delete name;
}

std::string Domain::validRequirement(std::string req) {
  if (validRequirements.find(req) == validRequirements.end()) {
    std::cerr << "Error! Invalid requirement: " << req << std::endl;
    exit(EXIT_FAILURE);
  }
  return req;

}

std::vector<ObjectDefine*>* Domain::getObjects() {
    return domainList->getObjects();
}

std::vector<DefineType*>* Domain::getDomainTypes() {
    return domainList->getTypes();
}

std::vector<PvarDefinition*>* Domain::getPvarDefinitions() {
    return domainList->getPVars();
}

std::vector<CpfDefinition*>* Domain::getCpfs() {
    return domainList->getCpfs();
}

LogicalExpression* Domain::getReward() {
    return domainList->getReward();
}

std::vector<LogicalExpression*>* Domain::getStateConstraints() {
    return domainList->getStateConstraints();
}

std::string Domain::getName() {
    return *name;
}

