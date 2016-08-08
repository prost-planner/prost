#include "rddl.h"

#include <iostream>
#include <fstream>

#include "logical_expressions.h"

#include "instantiator.h"
#include "preprocessor.h"
#include "evaluatables.h"
#include "task_analyzer.h"

#include "utils/timer.h"
#include "utils/system_utils.h"
#include "utils/string_utils.h"
#include "utils/math_utils.h"

std::map<std::string, PvarDefinition*> parametrizedVariableDefinitionsMap; // Map for storing definition of ParametrizedVariables
std::map<std::string, PvarExpression*> parametrizedVariableMap; // Map for storing ParametrizedVariables as expressions
std::map<std::string, Object*> objectMap; // Map for storing defined objects
std::map<std::string, Type*> typeMap; // Map for storing defined types

CpfDefinition::~CpfDefinition() {
    delete pVarExpression;
    delete logicalExpression;
}

std::string Domain::validRequirement(std::string req) {
    if (validRequirements.find(req) == validRequirements.end()) {
        std::cerr << "Error! Invalid requirement: " << req << std::endl;
        exit(EXIT_FAILURE);
    }
    return req;
}

/*****************************************************************
                           RDDL Block
*****************************************************************/
RDDLBlock::RDDLBlock() :
    numberOfConcurrentActions(std::numeric_limits<int>::max()),
    horizon(1),
    discountFactor(1.0),
    rewardCPF(nullptr),
    rewardLockDetected(false),
    unreasonableActionDetected(false),
    unreasonableActionInDeterminizationDetected(false),
    numberOfEncounteredStates(0),
    numberOfUniqueEncounteredStates(0),
    nonTerminalStatesWithUniqueAction(0),
    uniqueNonTerminalStatesWithUniqueAction(0)  {

    // Add bool type
    addType("bool");
    addObject("bool", "false");
    addObject("bool", "true");

    // Add numerical types
    addType("int");
    addType("real");

    // Add object super type
    addType("object");
    }

void addTypeSection(RDDLBlock* rddlBlock, Domain* domain) {
    if (domain->getDomainTypes() == NULL) {
        return;
    }

    // Adding TypeSection
    for (std::vector<DefineType*>::iterator it = domain->getDomainTypes()->begin(); it != domain->getDomainTypes()->end(); it++) {
        // Simple type definition (type_name : type)
        if ((*it)->getSuperTypeList() == NULL) {
            // std::cout << "Added type (from type section): " << (*it)->getName() << " of type " << (*it)->getSuperType() << std::endl;
            rddlBlock->addType((*it)->getName(), (*it)->getSuperType());
        }
        else {
        // Definition of type using enum values (type_name : @enum1, @enum2, ... , @enumN)
            rddlBlock->addType((*it)->getName());
            for(std::vector<std::string>::iterator jt = (*it)->getSuperTypeList()->begin(); jt != (*it)->getSuperTypeList()->end(); jt++){
                // std::cout << "Added object (from type section): " << (*it)->getName() << " of type " << *jt << std::endl;
                rddlBlock->addObject((*it)->getName(), (*jt));
            }
        }
    }
}

void addPVarSection(RDDLBlock* rddlBlock, Domain* domain) {
    if (domain->getPvarDefinitions() == NULL) {
        return;
    }

    // Adding PvarSection
    for(std::vector<PvarDefinition*>::iterator it = domain->getPvarDefinitions()->begin(); it != domain->getPvarDefinitions()->end(); it++) {
        std::string name = (*it)->getName();
        std::string varTypeName = (*it)->getVarType();
        std::string defaultVarType = (*it)->getDefaultVarType();
        std::string satisfactionType = (*it)->getSatisfactionType();
        std::string defaultVarValString = (*it)->getDefaultVarValue();
        double defaultVarValue;

        std::vector<Parameter*> params;
        for(std::vector<std::string>::iterator jt = (*it)->getParameters()->begin(); jt != (*it)->getParameters()->end(); jt++) {
            if (rddlBlock->types.find((*jt)) == rddlBlock->types.end()) {
                SystemUtils::abort("Undefined type " + *jt + " used as parameter in " + name + ".");
            }
            else {
                params.push_back(new Parameter(rddlBlock->types[*jt]->name, rddlBlock->types[*jt]));
            }
            // std::cout << "Adding parameter: " << rddlBlock->types[*jt]->name << " of type " << rddlBlock->types[*jt]->name << std::endl;

        }

        // TODO: This initialization here is wrong but is put here to prevent warning during the compliation
        // setting it to NULL doesn't help
        ParametrizedVariable::VariableType varType = ParametrizedVariable::STATE_FLUENT;
        if (varTypeName == "state-fluent") {
            varType = ParametrizedVariable::STATE_FLUENT;
        }
        else if (varTypeName == "action-fluent") {
            varType = ParametrizedVariable::ACTION_FLUENT;
        }
        else if (varTypeName == "non-fluent") {
            varType = ParametrizedVariable::NON_FLUENT;
        }
        // TODO: cover other types of parametrized variables
        else {
            SystemUtils::abort("Parametrized variable: " + varTypeName + " not implemented. Implemented for now: state-fluent, action-fluent, non-fluent.");
        }

        if (rddlBlock->types.find(defaultVarType) == rddlBlock->types.end()) {
            SystemUtils::abort("Unknown type " + defaultVarType + " defined.");
        }

        Type* valueType = rddlBlock->types[defaultVarType];

        switch (varType) {
            case ParametrizedVariable::NON_FLUENT:
            case ParametrizedVariable::STATE_FLUENT:
            case ParametrizedVariable::ACTION_FLUENT: {
                if (satisfactionType != "default") {
                    SystemUtils::abort("Unknown satisfaction type for parametrized variable " + varTypeName + ". Did you mean 'default'?");
                }

                // TODO: ?? -> || valueType->name == "bool")
                if (valueType->name == "int" || valueType->name == "real" ) {
                    defaultVarValue = std::atof(defaultVarValString.c_str());
                }
                else {
                    if (rddlBlock->objects.find(defaultVarValString) == rddlBlock->objects.end()) {
                        SystemUtils::abort("Default value " + defaultVarValString + " of variable " + name + " not defined.");
                    }

                    Object* val = rddlBlock->objects[defaultVarValString];
                    defaultVarValue = val->value;
                }
                break;
            }

            // case ParametrizedVariable::INTERM_FLUENT:
            // case ParametrizedVariable::DERIVED_FLUENT:
            //     if (satisfactionType != "level") {
            //         SystemUtils::abort("Unknown satisfaction type for parametrized variable " + varTypeName + ". Did you mean 'level'?");
            //     }
            //     // TODO: implement this
            //     SystemUtils::abort("Not implemented. Var type: " + varTypeName);
            //     break;

            default:
            {
                SystemUtils::abort("Unknown var type: " + varTypeName);
                break;
            }
        }

        // std::cout << "Adding parametrized variable: " << name << " default var type " << defaultVarType  << " default value: " << defaultVarValue << std::endl;
        ParametrizedVariable* var = new ParametrizedVariable(name, params, varType, valueType, defaultVarValue);

        // std::cout << "Adding parametrized variable in addPvarSection: " << name << " default var type " << defaultVarType  << " default value: " << defaultVarValue << std::endl;
        // for (unsigned i = 0; i < params.size(); i++)
        //     std::cout << "\t" << params[i]->name << std::endl;

        rddlBlock->addVariableDefinition(var);
    }

}

void addCpfSection(RDDLBlock* rddlBlock, Domain* domain) {
    if (domain->getCpfs() == NULL) {
        return;
    }

    // std::cout << "########## Adding " << domain->getCpfs()->size() << " CPF definitions" << std::endl;

    // Consists of Parametrized variable and logical expression
    for(std::vector<CpfDefinition*>::iterator it = domain->getCpfs()->begin(); it != domain->getCpfs()->end(); it++) {
        // P Var
        std::string name = (*it)->getPvar()->getName();

        // std::cout << "Pvar name: " << name << std::endl;

        if (name[name.length() - 1] == '\'') {
            name = name.substr(0, name.length() - 1);
        }

        if (rddlBlock->variableDefinitions.find(name) == rddlBlock->variableDefinitions.end()) {
            SystemUtils::abort("No according variable to CPF " + name + ".");
        }

        ParametrizedVariable* head = rddlBlock->variableDefinitions[name];

        if ((*it)->getPvar()->getParameters() == NULL) {
                if (head->params.size() != 0)
                    SystemUtils::abort("Wrong number of parameters for parametrized variable " + name + ".");
            }
        else if (head->params.size() != ((*it)->getPvar()->getParameters()->size())) {
                SystemUtils::abort("Wrong number of parameters for parametrized variable " + name + ".");
            }

        if ((*it)->getPvar()->getParameters() != NULL) {
            unsigned i = 0;
            for(std::vector<std::string>::iterator jt = (*it)->getPvar()->getParameters()->begin(); jt != (*it)->getPvar()->getParameters()->end(); jt++) {
                head->params[i++]->name = (*jt);
                // std::cout << "\tparameter: " << *jt << " of type " << head->params[i-1]->type->name << std::endl;
            }
        }

        if (rddlBlock->CPFDefinitions.find(head) != rddlBlock->CPFDefinitions.end()) {
            SystemUtils::abort("Error: Multiple definition of CPF " + name + ".");
        }

        // Expression
        rddlBlock->CPFDefinitions[head] = (*it)->getLogicalExpression();
    }
}

void addRewardSection(RDDLBlock* rddlBlock, Domain* domain) {
    if (domain->getReward() == NULL)
        return;

    rddlBlock->setRewardCPF(domain->getReward());
}

void addStateConstraint(RDDLBlock* rddlBlock, Domain* domain) {
    if (domain->getStateConstraints() == NULL) {
        return;
    }

    for(std::vector<LogicalExpression*>::iterator it = domain->getStateConstraints()->begin(); it != domain->getStateConstraints()->end(); it++) {
        rddlBlock->SACs.push_back(*it);
    }
}

void addObjectsSection(RDDLBlock* rddlBlock, Domain* domain) {
    if (domain->getObjects() == NULL) {
        return;
    }

    for (std::vector<ObjectDefine*>::iterator it = domain->getObjects()->begin(); it != domain->getObjects()->end(); it++) {
        std::string typeName = (*it)->getTypeName();
        if (rddlBlock->types.find(typeName) == rddlBlock->types.end()) {
            SystemUtils::abort("Unknown object " + typeName);
        }
        for (std::vector<std::string>::iterator jt = (*it)->getObjectNames()->begin(); jt != (*it)->getObjectNames()->end(); jt++) {
            rddlBlock->addObject(typeName, (*jt));
            std::cout << "Added object (from objects section): " << typeName << " : " << *jt << std::endl;
        }
    }
}

void RDDLBlock::addDomain(Domain* domain) {
    // TODO: requirements section is stored and prepared, implementation is left

    domainName = domain->getName();

    addTypeSection(this, domain);
    addPVarSection(this, domain);
    addCpfSection(this, domain);
    addRewardSection(this, domain);
    addStateConstraint(this, domain);
    addObjectsSection(this, domain);
    // TODO: StateConstraintsSection
    // TODO: ActionPreconditionsSection
    // TODO: StateInvariantSection
    // TODO: ObjectsSection -> this can be implemented already
    // std::cout << "Domain added." << std::endl;

}

void RDDLBlock::addNonFluent(NonFluentBlock* nonFluent) {
    // Set nonFluentsName
    nonFluentsName = nonFluent->getName();

    // Check if domain name is corresponding
    if (nonFluent->getDomainName() != this->getDomainName()) {
        SystemUtils::abort("Unknown domain " + nonFluent->getDomainName() + "  defined in Non fluent section");
    }

    // Adding objects
    for (std::vector<ObjectDefine*>::iterator it = nonFluent->getObjects()->begin(); it != nonFluent->getObjects()->end(); it++) {
        std::string typeName = (*it)->getTypeName();
        if (types.find(typeName) == types.end()) {
            SystemUtils::abort("Unknown object " + typeName);
        }
        for (std::vector<std::string>::iterator jt = (*it)->getObjectNames()->begin(); jt != (*it)->getObjectNames()->end(); jt++) {
            addObject(typeName, (*jt));
            // std::cout << "Added object (from non fluents): " << *jt << " of type " << typeName << std::endl;
        }
    }

    // Adding non fluents
    std::vector<Parameter*> params;
    for (std::vector<PvariablesInstanceDefine*>::iterator it = nonFluent->getNonFluents()->begin(); it != nonFluent->getNonFluents()->end(); it++) {
        std::string name = (*it)->getName();

        params.clear();
        // Set parameters
        if ((*it)->getLConstList() != NULL)
        {
            for(std::vector<std::string>::iterator jt = (*it)->getLConstList()->begin(); jt != (*it)->getLConstList()->end(); jt++) {
                std::string paramName = (*jt);
                if (objects.find(paramName) == objects.end()) {
                    SystemUtils::abort("Unknown object: " + paramName);
                }

                // std::cout << "Added non fluent (from non fluents): " << paramName << " of type " << objects[paramName]->type->name << std::endl;
                params.push_back(objects[paramName]); // TODO: this is wrong. It only covers the case that parametrized variable is a Parameter Expression
            }
        }

        if (variableDefinitions.find(name) == variableDefinitions.end()) {
            SystemUtils::abort("Variable " + name + " used but not defined.");
        }

        ParametrizedVariable* parent = variableDefinitions[name];
        double initialVal = (*it)->getInitValue();

        addParametrizedVariable(parent, params, initialVal);

        // std::cout << "###### Added parametrized variable (from addNonFluent) " << name << " with parameters: " << std::endl;
        // for (unsigned i = 0; i < params.size(); i++)
        //     std::cout << "\t" << params[i]->name << std::endl;
        // std::cout << "and value: " << initialVal << std::endl;
    }
    // std::cout << "NonFluents added." << std::endl;
}

void RDDLBlock::addInstance(Instance* instance) {
    // std::cout << "Adding instance" << std::endl;
    this->name = instance->getName();

    // Check domain name
    if (this->getDomainName() != instance->getDomainName()) {
        SystemUtils::abort("Unknown domain " + instance->getDomainName() + " defined in Instance section");
    }

    // Check Non fluents name
    if (this->getNonFluentsName() != instance->getNonFluentsName()) {
        SystemUtils::abort("Unknown non fluents " + instance->getNonFluentsName() + "defined in Non fluent section");
    }

    // Add init states
    if (instance->getPVariables() != NULL) {
        for (std::vector<PvariablesInstanceDefine*>::iterator it = instance->getPVariables()->begin(); it != instance->getPVariables()->end(); it++) {
            std::string name = (*it)->getName();
            std::vector<Parameter*> params;
            // Set parameters
            if ((*it)->getLConstList() != NULL)
            {
                for(std::vector<std::string>::iterator jt = (*it)->getLConstList()->begin(); jt != (*it)->getLConstList()->end(); jt++) {
                    std::string paramName = (*jt);
                    if (objects.find(paramName) == objects.end()) {
                        SystemUtils::abort("Unknown object: " + paramName);
                    }

                    params.push_back(objects[paramName]); // TODO: this is wrong. It only covers the case that parametrized variable is and Parameter Expression
                }

            }

            if (variableDefinitions.find(name) == variableDefinitions.end()) {
                SystemUtils::abort("Variable " + name + " used but not defined.");
            }

            ParametrizedVariable* parent = variableDefinitions[name];
            double initialVal = (*it)->getInitValue();

            addParametrizedVariable(parent, params, initialVal);

            // std::cout << "###### Added parametrized (from addInstance) " << name << " with parameters: " << std::endl;
            // for (unsigned i = 0; i < params.size(); i++)
            //     std::cout << "\t" << params[i]->name << std::endl;
            // std::cout << "and value: " << initialVal << std::endl;

        }
    }

    // Set Max nondef actions
    numberOfConcurrentActions = instance->getMaxNonDefActions();

    // Set horizon
    horizon = instance->getHorizon();

    // Set discount
    discountFactor = instance->getDiscount();

    // std::cout << "Instance added." << std::endl;
}

void RDDLBlock::addType(std::string const& name, std::string const& superType) {
    if (types.find(name) != types.end()) {
        SystemUtils::abort("Error: Type " + name + " is ambiguous.");
    }

    if (superType.empty()) {
        types[name] = new Type(name);
    } else if (types.find(superType) == types.end()) {
        SystemUtils::abort("Error: Supertype not found: " + superType);
    } else {
        types[name] = new Type(name, types[superType]);
    }
}

void RDDLBlock::addObject(std::string const& typeName,
                             std::string const& objectName) {
    if (types.find(typeName) == types.end()) {
        SystemUtils::abort("Error: Type " + typeName + " not defined.");
    }

    if (objects.find(objectName) != objects.end()) {
        SystemUtils::abort(
            "Error: Object name " + objectName + " is ambiguous.");
    }

    objects[objectName] = new Object(objectName, types[typeName]);
}

void RDDLBlock::addVariableDefinition(ParametrizedVariable* varDef) {
    if (variableDefinitions.find(varDef->fullName) !=
        variableDefinitions.end()) {
        SystemUtils::abort(
                "Error: Ambiguous variable name: " + varDef->fullName);
    }
    variableDefinitions[varDef->fullName] = varDef;
}

void RDDLBlock::addParametrizedVariable(ParametrizedVariable* parent,
                                           std::vector<Parameter*> const& params) {
    addParametrizedVariable(parent, params, parent->initialValue);
}

void RDDLBlock::addParametrizedVariable(ParametrizedVariable* parent,
                                           std::vector<Parameter*> const& params,
                                           double initialValue) {
    if (variableDefinitions.find(parent->variableName) == variableDefinitions.end()) {
        SystemUtils::abort(
                "Error: Parametrized variable " + parent->variableName +
                " not defined.");
    }

    switch(parent->variableType) {
    case ParametrizedVariable::STATE_FLUENT: {
        StateFluent* sf = new StateFluent(*parent, params, initialValue);

        // This is already defined if it occurs in the initial state entry
        if(stateFluentMap.find(sf->fullName) != stateFluentMap.end()) {
            return;
        }

        stateFluents.push_back(sf);
        stateFluentMap[sf->fullName] = sf;

        if(stateFluentsBySchema.find(parent) == stateFluentsBySchema.end()) {
            stateFluentsBySchema[parent] = std::vector<StateFluent*>();
        }
        stateFluentsBySchema[parent].push_back(sf);
        break;
    }
    case ParametrizedVariable::ACTION_FLUENT: {
        ActionFluent* af = new ActionFluent(*parent, params);

        assert(actionFluentMap.find(af->fullName) == actionFluentMap.end());

        af->index = actionFluents.size();
        actionFluents.push_back(af);
        actionFluentMap[af->fullName] = af;
        break;
    }
    case ParametrizedVariable::NON_FLUENT: {
        NonFluent* nf = new NonFluent(*parent, params, initialValue);

        // This is already defined if it occurs in the non fluents entry
        if(nonFluentMap.find(nf->fullName) != nonFluentMap.end()) {
            return;
        }

        nonFluents.push_back(nf);
        nonFluentMap[nf->fullName] = nf;
        break;
    }
        // case ParametrizedVariable::INTERM_FLUENT:
        // assert(false);
        // break;
    }
}

StateFluent* RDDLBlock::getStateFluent(std::string const& name) {
    if(stateFluentMap.find(name) == stateFluentMap.end()) {
        SystemUtils::abort("Error: state-fluent " + name + " used but not defined.");
        return nullptr;
    }
    return stateFluentMap[name];
}

ActionFluent* RDDLBlock::getActionFluent(std::string const& name) {
    if(actionFluentMap.find(name) == actionFluentMap.end()) {
        SystemUtils::abort("Error: action-fluent " + name + " used but not defined.");
        return nullptr;
    }
    return actionFluentMap[name];
}

NonFluent* RDDLBlock::getNonFluent(std::string const& name) {
    if(nonFluentMap.find(name) == nonFluentMap.end()) {
        SystemUtils::abort("Error: non-fluent " + name + " used but not defined.");
        return nullptr;
    }
    return nonFluentMap[name];
}

// TODO: Return const reference?
std::vector<StateFluent*> RDDLBlock::getStateFluentsOfSchema(ParametrizedVariable* schema) {
    assert(stateFluentsBySchema.find(schema) != stateFluentsBySchema.end());
    return stateFluentsBySchema[schema];
}

void RDDLBlock::setRewardCPF(LogicalExpression* const& rewardFormula) {
    if (rewardCPF) {
        SystemUtils::abort("Error: RewardCPF exists already.");
    }
    rewardCPF = new RewardFunction(rewardFormula);
}

void RDDLBlock::print(std::ostream& out) {
    // Set precision of doubles
    out.unsetf(std::ios::floatfield);
    out.precision(std::numeric_limits<double>::digits10);

    int firstProbabilisticVarIndex = (int) CPFs.size();
    bool deterministic = true;
    for (unsigned int i = 0; i < CPFs.size(); ++i) {
        if (CPFs[i]->isProbabilistic()) {
            firstProbabilisticVarIndex = i;
            deterministic = false;
            break;
        }
    }

    out << "#####TASK#####" << std::endl;
    out << "## name" << std::endl;
    out << name << std::endl;
    out << "## horizon" << std::endl;
    out << horizon << std::endl;
    out << "## discount factor" << std::endl;
    out << discountFactor << std::endl;
    out << "## number of action fluents" << std::endl;
    out << actionFluents.size() << std::endl;
    out << "## number of det state fluents" << std::endl;
    out << firstProbabilisticVarIndex << std::endl;
    out << "## number of prob state fluents" << std::endl;
    out << (CPFs.size() - firstProbabilisticVarIndex) << std::endl;
    out << "## number of preconds" << std::endl;
    out << actionPreconds.size() << std::endl;
    out << "## number of actions" << std::endl;
    out << actionStates.size() << std::endl;
    out << "## number of hashing functions" << std::endl;
    out << (actionPreconds.size() + CPFs.size() + 1) << std::endl;
    out << "## initial state" << std::endl;
    for (unsigned int i = 0; i < CPFs.size(); ++i) {
        out << CPFs[i]->getInitialValue() << " ";
    }
    out << std::endl;
    out << "## 1 if task is deterministic" << std::endl;
    out << deterministic << std::endl;
    out << "## 1 if state hashing possible" << std::endl;
    out << !stateHashKeys.empty() << std::endl;
    out << "## 1 if kleene state hashing possible" << std::endl;
    out << !kleeneStateHashKeyBases.empty() << std::endl;
    out << "## method to calculate the final reward" << std::endl;
    out << finalRewardCalculationMethod << std::endl;
    if (finalRewardCalculationMethod == "BEST_OF_CANDIDATE_SET") {
        out <<
        "## set of candidates to calculate final reward (first line is the number)"
            << std::endl;
        out << candidatesForOptimalFinalAction.size() << std::endl;
        for (unsigned int i = 0; i < candidatesForOptimalFinalAction.size();
             ++i) {
            out << candidatesForOptimalFinalAction[i] << " ";
        }
        out << std::endl;
    }
    out <<
    "## 1 if reward formula allows reward lock detection and a reward lock was found during task analysis"
        << std::endl;
    out << rewardLockDetected << std::endl;
    out << "## 1 if an unreasonable action was detected" << std::endl;
    out << unreasonableActionDetected << std::endl;
    out <<
    "## 1 if an unreasonable action was detected in the determinization" <<
    std::endl;
    out << unreasonableActionInDeterminizationDetected << std::endl;


    out << "## number of states that were encountered during task analysis" << std::endl;
    out << numberOfEncounteredStates << std::endl;
    out << "## number of unique states that were encountered during task analysis" << std::endl;
    out << numberOfUniqueEncounteredStates << std::endl;
    out << "## number of states with only one applicable reasonable action that were encountered during task analysis" << std::endl;
    out << nonTerminalStatesWithUniqueAction << std::endl;
    out << "## number of unique states with only one applicable reasonable action that were encountered during task analysis" << std::endl;
    out << uniqueNonTerminalStatesWithUniqueAction << std::endl;


    out << std::endl << std::endl << "#####ACTION FLUENTS#####" << std::endl;
    for (unsigned int i = 0; i < actionFluents.size(); ++i) {
        out << "## index" << std::endl;
        out << actionFluents[i]->index << std::endl;
        out << "## name" << std::endl;
        out << actionFluents[i]->fullName << std::endl;
        out << "## number of values" << std::endl;
        out << "2" << std::endl;
        out << "## values" << std::endl;
        out << "0 false" << std::endl << "1 true" << std::endl;
        out << std::endl;
    }

    out << std::endl << std::endl << "#####DET STATE FLUENTS AND CPFS#####" << std::endl;
    for (unsigned int index = 0; index < firstProbabilisticVarIndex; ++index) {
        assert(CPFs[index]->head->index == index);
        assert(!CPFs[index]->isProbabilistic());
        out << "## index" << std::endl;
        out << index << std::endl;
        out << "## name" << std::endl;
        out << CPFs[index]->head->fullName << std::endl;
        out << "## number of values" << std::endl;
        out << CPFs[index]->domain.size() << std::endl;
        out << "## values" << std::endl;
        for (std::set<double>::iterator it = CPFs[index]->domain.begin();
             it != CPFs[index]->domain.end(); ++it) {
            out << *it << " " <<
            CPFs[index]->head->valueType->objects[*it]->name << std::endl;
        }

        out << "## formula" << std::endl;
        CPFs[index]->formula->print(out);
        out << std::endl;

        out << "## hash index" << std::endl;
        out << CPFs[index]->hashIndex << std::endl;
        out << "## caching type " << std::endl;
        out << CPFs[index]->cachingType << std::endl;
        if (CPFs[index]->cachingType == "VECTOR") {
            out << "## precomputed results" << std::endl;
            out << CPFs[index]->precomputedResults.size() << std::endl;
            for (unsigned int res = 0;
                 res < CPFs[index]->precomputedResults.size(); ++res) {
                out << res << " " << CPFs[index]->precomputedResults[res] <<
                std::endl;
            }
        }
        out << "## kleene caching type" << std::endl;
        out << CPFs[index]->kleeneCachingType << std::endl;
        if (CPFs[index]->kleeneCachingType == "VECTOR") {
            out << "## kleene caching vec size" << std::endl;
            out << CPFs[index]->kleeneCachingVectorSize << std::endl;
        }

        out << "## action hash keys" << std::endl;
        for (unsigned int actionIndex = 0;
             actionIndex < CPFs[index]->actionHashKeyMap.size();
             ++actionIndex) {
            out << actionIndex << " " <<
            CPFs[index]->actionHashKeyMap[actionIndex] << std::endl;
        }
        out << std::endl;
    }

    out << std::endl << std::endl << "#####PROB STATE FLUENTS AND CPFS#####" << std::endl;
    for (unsigned int index = firstProbabilisticVarIndex; index < CPFs.size(); ++index) {
        assert(CPFs[index]->head->index == index);
        assert(CPFs[index]->isProbabilistic());
        out << "## index" << std::endl;
        out << (index - firstProbabilisticVarIndex) << std::endl;
        out << "## name" << std::endl;
        out << CPFs[index]->head->fullName << std::endl;
        out << "## number of values" << std::endl;
        out << CPFs[index]->domain.size() << std::endl;
        out << "## values" << std::endl;
        for (std::set<double>::iterator it = CPFs[index]->domain.begin();
             it != CPFs[index]->domain.end(); ++it) {
            out << *it << " " <<
            CPFs[index]->head->valueType->objects[*it]->name << std::endl;
        }

        out << "## formula" << std::endl;
        CPFs[index]->formula->print(out);
        out << std::endl;

        out << "## determinized formula" << std::endl;
        CPFs[index]->determinization->print(out);
        out << std::endl;

        out << "## hash index" << std::endl;
        out << CPFs[index]->hashIndex << std::endl;
        out << "## caching type " << std::endl;
        out << CPFs[index]->cachingType << std::endl;
        if (CPFs[index]->cachingType == "VECTOR") {
            out <<
            "## precomputed results (key - determinization - size of distribution - value-probability pairs)"
                << std::endl;
            out << CPFs[index]->precomputedResults.size() << std::endl;
            for (unsigned int res = 0;
                 res < CPFs[index]->precomputedResults.size(); ++res) {
                out << res << " " << CPFs[index]->precomputedResults[res] <<
                " " << CPFs[index]->precomputedPDResults[res].values.size();
                for (unsigned int valProbPair = 0;
                     valProbPair <
                     CPFs[index]->precomputedPDResults[res].values.size();
                     ++valProbPair) {
                    out << " " <<
                    CPFs[index]->precomputedPDResults[res].values[valProbPair]
                        << " " <<
                    CPFs[index]->precomputedPDResults[res].probabilities[
                        valProbPair];
                }
                out << std::endl;
            }
        }
        out << "## kleene caching type" << std::endl;
        out << CPFs[index]->kleeneCachingType << std::endl;
        if (CPFs[index]->kleeneCachingType == "VECTOR") {
            out << "## kleene caching vec size" << std::endl;
            out << CPFs[index]->kleeneCachingVectorSize << std::endl;
        }

        out << "## action hash keys" << std::endl;
        for (unsigned int actionIndex = 0;
             actionIndex < CPFs[index]->actionHashKeyMap.size();
             ++actionIndex) {
            out << actionIndex << " " <<
            CPFs[index]->actionHashKeyMap[actionIndex] << std::endl;
        }

        out << std::endl;
    }

    out << std::endl << std::endl << "#####REWARD#####" << std::endl;
    out << "## formula" << std::endl;
    rewardCPF->formula->print(out);
    out << std::endl;
    out << "## min" << std::endl;
    out << *rewardCPF->domain.begin() << std::endl;
    out << "## max" << std::endl;
    out << *rewardCPF->domain.rbegin() << std::endl;
    out << "## independent from actions" << std::endl;
    out << (rewardCPF->positiveActionDependencies.empty() && rewardCPF->negativeActionDependencies.empty()) << std::endl;
    out << "## hash index" << std::endl;
    out << rewardCPF->hashIndex << std::endl;
    out << "## caching type" << std::endl;
    out << rewardCPF->cachingType << std::endl;
    if (rewardCPF->cachingType == "VECTOR") {
        out << "## precomputed results" << std::endl;
        out << rewardCPF->precomputedResults.size() << std::endl;
        for (unsigned int res = 0; res < rewardCPF->precomputedResults.size();
             ++res) {
            out << res << " " << rewardCPF->precomputedResults[res] << std::endl;
        }
    }
    out << "## kleene caching type" << std::endl;
    out << rewardCPF->kleeneCachingType << std::endl;
    if (rewardCPF->kleeneCachingType == "VECTOR") {
        out << "## kleene caching vec size" << std::endl;
        out << rewardCPF->kleeneCachingVectorSize << std::endl;
    }

    out << "## action hash keys" << std::endl;
    for (unsigned int actionIndex = 0;
         actionIndex < rewardCPF->actionHashKeyMap.size(); ++actionIndex) {
        out << actionIndex << " " <<
        rewardCPF->actionHashKeyMap[actionIndex] << std::endl;
    }

    out << std::endl << std::endl << "#####PRECONDITIONS#####" << std::endl;

    for (unsigned int index = 0; index < actionPreconds.size(); ++index) {
        assert(actionPreconds[index]->index == index);
        out << "## index" << std::endl;
        out << index << std::endl;
        out << "## formula" << std::endl;
        actionPreconds[index]->formula->print(out);
        out << std::endl;
        out << "## hash index" << std::endl;
        out << actionPreconds[index]->hashIndex << std::endl;
        out << "## caching type" << std::endl;
        out << actionPreconds[index]->cachingType << std::endl;
        if (actionPreconds[index]->cachingType == "VECTOR") {
            out << "## precomputed results" << std::endl;
            out << actionPreconds[index]->precomputedResults.size() << std::endl;
            for (unsigned int res = 0;
                 res < actionPreconds[index]->precomputedResults.size(); ++res) {
                out << res << " " <<
                actionPreconds[index]->precomputedResults[res] << std::endl;
            }
        }
        out << "## kleene caching type" << std::endl;
        out << actionPreconds[index]->kleeneCachingType << std::endl;
        if (actionPreconds[index]->kleeneCachingType == "VECTOR") {
            out << "## kleene caching vec size" << std::endl;
            out << actionPreconds[index]->kleeneCachingVectorSize << std::endl;
        }

        out << "## action hash keys" << std::endl;
        for (unsigned int actionIndex = 0;
             actionIndex < actionPreconds[index]->actionHashKeyMap.size();
             ++actionIndex) {
            out << actionIndex << " " <<
            actionPreconds[index]->actionHashKeyMap[actionIndex] << std::endl;
        }

        out << std::endl;
    }

    out << std::endl << std::endl << "#####ACTION STATES#####" << std::endl;
    for (unsigned int index = 0; index < actionStates.size(); ++index) {
        assert(index == actionStates[index].index);
        out << "## index" << std::endl;
        out << index << std::endl;
        out << "## state" << std::endl;
        for (unsigned int varIndex = 0;
             varIndex < actionStates[index].state.size(); ++varIndex) {
            out << actionStates[index][varIndex] << " ";
        }
        out << std::endl;
        out << "## relevant preconditions" << std::endl;
        out << actionStates[index].relevantSACs.size() << std::endl;
        for (unsigned int sacIndex = 0;
             sacIndex < actionStates[index].relevantSACs.size(); ++sacIndex) {
            out << actionStates[index].relevantSACs[sacIndex]->index << " ";
        }
        out << std::endl;
        out << std::endl;
    }

    out << std::endl << "#####HASH KEYS OF DETERMINISTIC STATE FLUENTS#####" << std::endl;
    for (unsigned int index = 0; index < firstProbabilisticVarIndex; ++index) {
        assert(CPFs[index]->head->index == index);
        out << "## index" << std::endl;
        out << index << std::endl;
        if (!stateHashKeys.empty()) {
            out << "## state hash key (for each value in the domain)" << std::endl;
            for (unsigned int valIndex = 0;
                 valIndex < stateHashKeys[index].size(); ++valIndex) {
                out << stateHashKeys[index][valIndex];
                if (valIndex != stateHashKeys[index].size() - 1) {
                    out << " ";
                }
            }
        }
        out << std::endl;

        if (!kleeneStateHashKeyBases.empty()) {
            out << "## kleene state hash key base" << std::endl;
            out << kleeneStateHashKeyBases[index] << std::endl;
        }

        out <<
        "## state fluent hash keys (first line is the number of keys)" << std::endl;
        out << indexToStateFluentHashKeyMap[index].size() << std::endl;
        for (unsigned int i = 0; i < indexToStateFluentHashKeyMap[index].size();
             ++i) {
            out << indexToStateFluentHashKeyMap[index][i].first << " ";
            out << indexToStateFluentHashKeyMap[index][i].second << std::endl;
        }

        out <<
        "## kleene state fluent hash keys (first line is the number of keys)"
            <<
        std::endl;
        out << indexToKleeneStateFluentHashKeyMap[index].size() << std::endl;
        for (unsigned int i = 0;
             i < indexToKleeneStateFluentHashKeyMap[index].size(); ++i) {
            out << indexToKleeneStateFluentHashKeyMap[index][i].first << " ";
            out << indexToKleeneStateFluentHashKeyMap[index][i].second << std::endl;
        }
        out << std::endl;
    }

    out << std::endl << "#####HASH KEYS OF PROBABILISTIC STATE FLUENTS#####" << std::endl;
    for (unsigned int index = firstProbabilisticVarIndex; index < CPFs.size();
         ++index) {
        assert(CPFs[index]->head->index == index);
        out << "## index" << std::endl;
        out << (index - firstProbabilisticVarIndex) << std::endl;
        if (!stateHashKeys.empty()) {
            out << "## state hash key (for each value in the domain)" << std::endl;
            for (unsigned int valIndex = 0;
                 valIndex < stateHashKeys[index].size(); ++valIndex) {
                out << stateHashKeys[index][valIndex];
                if (valIndex != stateHashKeys[index].size() - 1) {
                    out << " ";
                }
            }
        }
        out << std::endl;

        if (!kleeneStateHashKeyBases.empty()) {
            out << "## kleene state hash key base" << std::endl;
            out << kleeneStateHashKeyBases[index] << std::endl;
        }

        out <<
        "## state fluent hash keys (first line is the number of keys)" << std::endl;
        out << indexToStateFluentHashKeyMap[index].size() << std::endl;
        for (unsigned int i = 0; i < indexToStateFluentHashKeyMap[index].size();
             ++i) {
            out << indexToStateFluentHashKeyMap[index][i].first << " ";
            out << indexToStateFluentHashKeyMap[index][i].second << std::endl;
        }

        out <<
        "## kleene state fluent hash keys (first line is the number of keys)"
            <<
        std::endl;
        out << indexToKleeneStateFluentHashKeyMap[index].size() << std::endl;
        for (unsigned int i = 0;
             i < indexToKleeneStateFluentHashKeyMap[index].size(); ++i) {
            out << indexToKleeneStateFluentHashKeyMap[index][i].first << " ";
            out << indexToKleeneStateFluentHashKeyMap[index][i].second << std::endl;
        }
        out << std::endl;
    }

    out << std::endl << std::endl << "#####TRAINING SET#####" << std::endl;
    out << trainingSet.size() << std::endl;
    for (std::set<State>::iterator it = trainingSet.begin(); it != trainingSet.end();
         ++it) {
        for (unsigned int i = 0; i < it->state.size(); ++i) {
            out << it->state[i] << " ";
        }
        out << std::endl;
    }
}

void RDDLBlock::execute(std::string td/*target dir*/) {
    std::cout << "Executing..." << std::endl << "Writing output.." << std::endl;

    Timer t, totalTime;
    std::cout << "Parsing...finished" << std::endl;

    t.reset();
    std::cout << "instantiating..." << std::endl;
    Instantiator instantiator(this);
    instantiator.instantiate();
    std::cout << "...finished (" << t << ")." << std::endl;

    t.reset();
    std::cout << "preprocessing..." << std::endl;
    Preprocessor preprocessor(this);
    preprocessor.preprocess();
    std::cout << "...finished (" << t << ")." << std::endl;

    t.reset();
    std::cout << "analyzing task..." << std::endl;
    TaskAnalyzer analyzer(this);
    analyzer.analyzeTask();
    std::cout << "...finished (" << t << ")." << std::endl;

    t.reset();
    std::cout << "writing output..." << std::endl;
    std::ofstream resultFile;
    std::cout << "Task name " << name << std::endl;
    std::string targetDir = td + "/" + name;
    resultFile.open(targetDir.c_str());
    print(resultFile);
    resultFile.close();
    // print(std::cout);
    std::cout << "...finished (" << t << ")." << name << std::endl;

    std::cout << "total time: " << totalTime << std::endl;
}


/*****************************************************************
                           Helper Methods
*****************************************************************/

// This method is used to get the value of parametrized variable
// Parametrized variable is stored with its definition in pVarDefinition and the
// values of parameters used in the particular call of parametrized variable are stored in pVarExpression
ParametrizedVariable* getParametrizedVariableFromPvarDefinition(std::string pVarName) {

    if (parametrizedVariableDefinitionsMap.find(pVarName) == parametrizedVariableDefinitionsMap.end())
        return NULL;

    PvarDefinition* pVarDefinition = parametrizedVariableDefinitionsMap[pVarName];
    PvarExpression* pVarExpression = parametrizedVariableMap[pVarName];

    std::string name = pVarDefinition->getName();
    std::string varTypeName = pVarDefinition->getVarType();
    std::string defaultVarType = pVarDefinition->getDefaultVarType();
    std::string satisfactionType = pVarDefinition->getSatisfactionType();
    std::string defaultVarValString = pVarDefinition->getDefaultVarValue();
    double defaultVarValue;

    std::vector<Parameter*> params;
    // Adding parameters from PvarExpression (those are the parameters that user set when he called parametrized variablea as an espression)
    for(unsigned i = 0; i < pVarDefinition->getParameters()->size(); i++)
        if (typeMap.find((*pVarDefinition->getParameters())[i]) == typeMap.end()) {
            SystemUtils::abort("Undefined type " + (*pVarExpression->getParameters())[i] + " used as parameter in " + name + ".");
        }
        else {
            params.push_back(new Parameter((*pVarExpression->getParameters())[i], typeMap[(*pVarDefinition->getParameters())[i]]));
        }

    // TODO: This initialization here is wrong but is put here to prevent warning during the compliation
    // setting it to NULL doesn't help
    ParametrizedVariable::VariableType varType = ParametrizedVariable::STATE_FLUENT;
    if (varTypeName == "state-fluent") {
        varType = ParametrizedVariable::STATE_FLUENT;
    }
    else if (varTypeName == "action-fluent") {
        varType = ParametrizedVariable::ACTION_FLUENT;
    }
    else if (varTypeName == "non-fluent") {
        varType = ParametrizedVariable::NON_FLUENT;
    }
    // TODO: cover other types of parametrized variables
    else {
        SystemUtils::abort("Parametrized variable: " + varTypeName + " not implemented. Implemented for now: state-fluent, action-fluent, non-fluent.");
    }

    if (typeMap.find(defaultVarType) == typeMap.end()) {
        SystemUtils::abort("Unknown type " + defaultVarType + " defined.");
    }
    Type* valueType = typeMap[defaultVarType];

    switch (varType) {
        case ParametrizedVariable::NON_FLUENT:
        case ParametrizedVariable::STATE_FLUENT:
        case ParametrizedVariable::ACTION_FLUENT:
            if (satisfactionType != "default") {
                SystemUtils::abort("Unknown satisfaction type for parametrized variable " + varTypeName + ". Did you mean 'default'?");
            }

            if (valueType->name == "int" || valueType->name == "real" ) {// TODO: ?? -> || valueType->name == "bool")
                defaultVarValue = std::atof(defaultVarValString.c_str());
            }
            else {
                if (objectMap.find(defaultVarValString) == objectMap.end()) {
                    SystemUtils::abort("Default value " + defaultVarValString + " of variable " + name + " not defined.");
                }
                Object* val = objectMap[defaultVarValString];
                defaultVarValue = val->value;
            }
            break;

        // case ParametrizedVariable::INTERM_FLUENT:
        // case ParametrizedVariable::DERIVED_FLUENT:
        //     if (satisfactionType != "level")
        //         SystemUtils::abort("Unknown satisfaction type for parametrized variable " + varTypeName + ". Did you mean 'level'?");
        //
        //     // TODO: implement this
        //     SystemUtils::abort("Not implemented. Var type: " + varTypeName);
        //     break;

        default:
            SystemUtils::abort("Unknown var type: " + varTypeName);
            break;
    }
    ParametrizedVariable* var = new ParametrizedVariable(name, params, varType, valueType, defaultVarValue);

    return var;
}

void storeParametrizedVariableFromPvarDefinition(std::string pVarName, PvarDefinition* pVar) {
    parametrizedVariableDefinitionsMap[pVarName] = pVar;
}

void storeParametrizedVariableMap(std::string pVarName, PvarExpression* pVarExpression) {
    parametrizedVariableMap[pVarName] = pVarExpression;
}

bool storeObject(std::string objName, std::string objectType) {

    if (objectMap.find(objName) != objectMap.end()) {
        return false;
    }

    objectMap[objName] = new Object(objName, typeMap[objectType]); // TODO: Should check if type exists. For some reason, simple check gives worng results.

    return true;
}

Object* getObject(std::string objName) {
    if (objectMap.find(objName) != objectMap.end()) {
        return objectMap[objName];
    }
    else {
       return NULL;
   }
}

bool storeType(std::string typeName, std::string superTypeName) {

    if (typeMap.find(superTypeName) != typeMap.end()) {
        typeMap[typeName] = new Type(typeName, typeMap[superTypeName]);
    }
    else {
        typeMap[typeName] = new Type(typeName);
    }

    return true;
}

Type* getType(std::string typeName) {
    if (typeMap.find(typeName) != typeMap.end()) {
        return typeMap[typeName];
    }
    else {
        return NULL;
    }
}


