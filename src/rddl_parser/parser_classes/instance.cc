/**
    instance.cc: implementatio of methods defined in instance.h

    @author Đorđe Relić <dorde.relic@unibas.ch>
    @version 1.0 06/2016
*/

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

