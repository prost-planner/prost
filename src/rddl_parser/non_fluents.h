/**
    non_fluents.h: classes needed for definition of non fluents section from rddl file

    @author Đorđe Relić <dorde.relic@unibas.ch>
    @version 1.0 06/2016
*/

#ifndef NONFLUENTS_H
#define NONFLUENTS_H
#include <string>
#include <vector>
#include <set>
#include <iostream>

#include "logical_expressions.h"


class PvariablesInstanceDefine {
public:
    PvariablesInstanceDefine(std::string _name, double _value, std::vector<std::string> *_lConstList = nullptr)
        :name(_name), initValue(_value), lConstList(_lConstList) { }// TODO: delete this : std::cout << "Instanciated PvariablesInstanceDefine : " << _name << " with parameter size: " << " and default value: " << _value << std::endl;}
    ~PvariablesInstanceDefine();
    std::string getName();
    std::vector<std::string>* getLConstList();
    double getInitValue();
private:
    std::string name; // TODO: if (task->variableDefinitions.find(name) == task->variableDefinitions.end()) { SystemUtils::abort("Error: variable " + name + " used but not defined.");  } else     ParametrizedVariable* parent = task->variableDefinitions[name]; ParametrizedVariable* parent = task->variableDefinitions[name];
    double initValue;
    std::vector<std::string> *lConstList;
};


class ObjectDefine {
public:
    ObjectDefine(std::string _typeName, std::vector<std::string> *_objectNames)
        :typeName(_typeName), objectNames(_objectNames) {}
    ~ObjectDefine();
    std::string getTypeName();
    std::vector<std::string>* getObjectNames();
private:
    std::string typeName;
    std::vector<std::string> *objectNames;
};


class NonFluentBlock {
public:
    NonFluentBlock(std::string _name, std::string _domainName, std::vector<PvariablesInstanceDefine*> *_nonFluents, std::vector<ObjectDefine*> *_objects = nullptr)
        :name(_name), domainName(_domainName), nonFluents(_nonFluents), objects(_objects) {}
    ~NonFluentBlock();
    std::string getName();
    std::string getDomainName();
    std::vector<ObjectDefine*>* getObjects();
    std::vector<PvariablesInstanceDefine*>* getNonFluents();
private:
    std::string name;
    std::string domainName;
    std::vector<PvariablesInstanceDefine*> *nonFluents;
    std::vector<ObjectDefine*> *objects;
};


#endif