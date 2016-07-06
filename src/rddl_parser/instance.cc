#include "instance.h"

// Instance
Instance::~Instance() {
    delete pVariables;
}

std::string Instance::getName(){
    return name;
}

std::string Instance::getDomainName() {
    return domainName;
}

std::string Instance::getNonFluentsName() {
    return nonFluentsName;
}

std::vector<PvariablesInstanceDefine*>* Instance::getPVariables() {
    return pVariables;
}

int Instance::getMaxNonDefActions() {
    return maxNonDefActions;
}

int Instance::getHorizon() {
    return horizon;
}

double Instance::getDiscount() {
    return discount;
}

