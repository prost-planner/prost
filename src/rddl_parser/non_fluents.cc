#include "non_fluents.h"

// PvariablesInstanceDefine
PvariablesInstanceDefine::~PvariablesInstanceDefine(){
    delete lConstList;
}

std::string PvariablesInstanceDefine::getName() {
    return name;
}

std::vector<std::string>* PvariablesInstanceDefine::getLConstList() {
    return lConstList;
}

double PvariablesInstanceDefine::getInitValue() {
    return initValue;
}

// DefineObject
ObjectDefine::~ObjectDefine() {
    delete objectNames;
}

std::string ObjectDefine::getTypeName() {
    return typeName;
}

std::vector<std::string>* ObjectDefine::getObjectNames() {
    return objectNames;
}


// Non Fluents
NonFluentBlock::~NonFluentBlock() {
    delete objects;
    delete nonFluents;
}

std::string NonFluentBlock::getName() {
    return name;
}

std::string NonFluentBlock::getDomainName() {
    return domainName;
}

std::vector<ObjectDefine*>* NonFluentBlock::getObjects() {
    return objects;
}

std::vector<PvariablesInstanceDefine*>* NonFluentBlock::getNonFluents() {
    return nonFluents;
}
