#ifndef INSTANCE_H
#define INSTANCE_H

#include <string>
#include <vector>
#include <iostream>
#include "non_fluents.h"

// new Instance(std::string domainName, std::string nonFluentsName, PvariablesInstanceList, int PositiveIntOrPositiveInfinity, int  horizon)
// { $$ = new Instance(*$6, *10, $14, $19, $21, $25);
class Instance {
public:
    Instance(std::string _name, std::string _domainName, std::string _nonFluentsName, std::vector<PvariablesInstanceDefine*> *_pVariables, int _maxNonDefAction, int _horizon, double _discount)
        :name(_name), domainName(_domainName), nonFluentsName(_nonFluentsName), pVariables(_pVariables), maxNonDefActions(_maxNonDefAction), horizon(_horizon), discount(_discount) {}
    ~Instance();
    std::string getName();
    std::string getDomainName();
    std::string getNonFluentsName();
    std::vector<PvariablesInstanceDefine*>* getPVariables();
    int getMaxNonDefActions();
    int getHorizon();
    double getDiscount();
private:
    std::string name;
    std::string domainName;
    std::string nonFluentsName;
    std::vector<PvariablesInstanceDefine*> *pVariables;
    int maxNonDefActions;
    int horizon;
    double discount;
};

#endif