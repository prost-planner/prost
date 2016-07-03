/**
    rddl_block.cc: implementation of methods from rddl_block.h

    @author Đorđe Relić <dorde.relic@unibas.ch>
    @version 1.0 06/2016
*/

#include "rddl_block.h"


RDDLBlock::~RDDLBlock() {}

std::string RDDLBlock::getDomainName() {
    return domainName;
}

std::string RDDLBlock::getNonFluentsName() {
    return nonFluentsName;
}

void addTypeSection(RDDLBlock *rddlBlock, Domain* domain) {
    if (domain->getDomainTypes() == NULL)
        return;

    // Adding TypeSection
    for (std::vector<DefineType*>::iterator it = domain->getDomainTypes()->begin(); it != domain->getDomainTypes()->end(); it++) {
        // Simple type definition (type_name : type)
        if ((*it)->getSuperTypeList() == NULL) {
            // std::cout << "Added type (from type section): " << (*it)->getName() << " of type " << (*it)->getSuperType() << std::endl;
            rddlBlock->task->addType((*it)->getName(), (*it)->getSuperType());
        }
        else {
        // Definition of type using enum values (type_name : @enum1, @enum2, ... , @enumN)
            rddlBlock->task->addType((*it)->getName());
            for(std::vector<std::string>::iterator jt = (*it)->getSuperTypeList()->begin(); jt != (*it)->getSuperTypeList()->end(); jt++){
                // std::cout << "Added object (from type section): " << (*it)->getName() << " of type " << *jt << std::endl;
                rddlBlock->task->addObject((*it)->getName(), (*jt));
            }
        }
    }
}

void addPVarSection(RDDLBlock *rddlBlock, Domain *domain) {
    if (domain->getPvarDefinitions() == NULL)
        return;

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
            if (rddlBlock->task->types.find((*jt)) == rddlBlock->task->types.end())
                SystemUtils::abort("Undefined type " + *jt + " used as parameter in " + name + ".");
            else
                params.push_back(new Parameter(rddlBlock->task->types[*jt]->name, rddlBlock->task->types[*jt]));
                // params.push_back(new Parameter(*jt, rddlBlock->task->types[*jt]));

            // std::cout << "Adding parameter: " << rddlBlock->task->types[*jt]->name << " of type " << rddlBlock->task->types[*jt]->name << std::endl;

        }

        // TODO: This initialization here is wrong but is put here to prevent warning during the compliation
        // setting it to NULL doesn't help
        ParametrizedVariable::VariableType varType = ParametrizedVariable::STATE_FLUENT;
        if (varTypeName == "state-fluent")
            varType = ParametrizedVariable::STATE_FLUENT;
        else if (varTypeName == "action-fluent")
            varType = ParametrizedVariable::ACTION_FLUENT;
        else if (varTypeName == "non-fluent")
            varType = ParametrizedVariable::NON_FLUENT;
        // TODO: cover other types of parametrized variables
        else
            SystemUtils::abort("Parametrized variable: " + varTypeName + " not implemented. Implemented for now: state-fluent, action-fluent, non-fluent.");

        if (rddlBlock->task->types.find(defaultVarType) == rddlBlock->task->types.end())
            SystemUtils::abort("Unknown type " + defaultVarType + " defined.");
        Type* valueType = rddlBlock->task->types[defaultVarType];

        switch (varType) {
            case ParametrizedVariable::NON_FLUENT:
            case ParametrizedVariable::STATE_FLUENT:
            case ParametrizedVariable::ACTION_FLUENT:
                if (satisfactionType != "default")
                    SystemUtils::abort("Unknown satisfaction type for parametrized variable " + varTypeName + ". Did you mean 'default'?");

                if (valueType->name == "int" || valueType->name == "real" ) // TODO: ?? -> || valueType->name == "bool")
                    defaultVarValue = std::atof(defaultVarValString.c_str());
                else {
                    if (rddlBlock->task->objects.find(defaultVarValString) == rddlBlock->task->objects.end())
                        SystemUtils::abort("Default value " + defaultVarValString + " of variable " + name + " not defined.");
                    Object* val = rddlBlock->task->objects[defaultVarValString];
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
        // std::cout << "Adding parametrized variable: " << name << " default var type " << defaultVarType  << " default value: " << defaultVarValue << std::endl;
        ParametrizedVariable* var = new ParametrizedVariable(name, params, varType, valueType, defaultVarValue);

        // std::cout << "Adding parametrized variable in addPvarSection: " << name << " default var type " << defaultVarType  << " default value: " << defaultVarValue << std::endl;
        // for (unsigned i = 0; i < params.size(); i++)
        //     std::cout << "\t" << params[i]->name << std::endl;

        rddlBlock->task->addVariableDefinition(var);
    }

}

void addCpfSection(RDDLBlock *rddlBlock, Domain *domain) {
    if (domain->getCpfs() == NULL)
        return;

    // std::cout << "########## Adding " << domain->getCpfs()->size() << " CPF definitions" << std::endl;

    // Consists of Parametrized variable and logical expression
    for(std::vector<CpfDefinition*>::iterator it = domain->getCpfs()->begin(); it != domain->getCpfs()->end(); it++) {
        // P Var
        std::string name = (*it)->getPvar()->getName();

        // std::cout << "Pvar name: " << name << std::endl;

        if (name[name.length() - 1] == '\'')
            name = name.substr(0, name.length() - 1);

        if (rddlBlock->task->variableDefinitions.find(name) == rddlBlock->task->variableDefinitions.end())
            SystemUtils::abort("No according variable to CPF " + name + ".");

        ParametrizedVariable* head = rddlBlock->task->variableDefinitions[name];

        if ((*it)->getPvar()->getParameters() == NULL) {
                if (head->params.size() != 0)
                    SystemUtils::abort("Wrong number of parameters for parametrized variable " + name + ".");
            }
        else if (head->params.size() != ((*it)->getPvar()->getParameters()->size()))
                SystemUtils::abort("Wrong number of parameters for parametrized variable " + name + ".");

        if ((*it)->getPvar()->getParameters() != NULL) {
            unsigned i = 0;
            for(std::vector<std::string>::iterator jt = (*it)->getPvar()->getParameters()->begin(); jt != (*it)->getPvar()->getParameters()->end(); jt++) {
                head->params[i++]->name = (*jt);
                // std::cout << "\tparameter: " << *jt << " of type " << head->params[i-1]->type->name << std::endl;
            }
        }

        if (rddlBlock->task->CPFDefinitions.find(head) != rddlBlock->task->CPFDefinitions.end())
            SystemUtils::abort("Error: Multiple definition of CPF " + name + ".");

        // Expression
        rddlBlock->task->CPFDefinitions[head] = (*it)->getLogicalExpression();
    }
}

void addRewardSection(RDDLBlock *rddlBlock, Domain *domain) {
    if (domain->getReward() == NULL)
        return;

    rddlBlock->task->setRewardCPF(domain->getReward());
}

void addStateConstraint(RDDLBlock *rddlBlock, Domain *domain) {
    if (domain->getStateConstraints() == NULL)
        return;

    for(std::vector<LogicalExpression*>::iterator it = domain->getStateConstraints()->begin(); it != domain->getStateConstraints()->end(); it++)
        rddlBlock->task->SACs.push_back(*it);
}

void addObjectsSection(RDDLBlock *rddlBlock, Domain *domain) {
    if (domain->getObjects() == NULL)
        return;

    for (std::vector<ObjectDefine*>::iterator it = domain->getObjects()->begin(); it != domain->getObjects()->end(); it++) {
        std::string typeName = (*it)->getTypeName();
        if (rddlBlock->task->types.find(typeName) == rddlBlock->task->types.end())
            SystemUtils::abort("Unknown object " + typeName);
        for (std::vector<std::string>::iterator jt = (*it)->getObjectNames()->begin(); jt != (*it)->getObjectNames()->end(); jt++) {
            rddlBlock->task->addObject(typeName, (*jt));
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

void RDDLBlock::addNonFluent(NonFluentBlock *nonFluent) {
    // Set nonFluentsName
    nonFluentsName = nonFluent->getName();

    // Check if domain name is corresponding
    if (nonFluent->getDomainName() != this->getDomainName())
        SystemUtils::abort("Unknown domain " + nonFluent->getDomainName() + "  defined in Non fluent section");

    // Adding objects
    for (std::vector<ObjectDefine*>::iterator it = nonFluent->getObjects()->begin(); it != nonFluent->getObjects()->end(); it++) {
        std::string typeName = (*it)->getTypeName();
        if (task->types.find(typeName) == task->types.end())
            SystemUtils::abort("Unknown object " + typeName);
        for (std::vector<std::string>::iterator jt = (*it)->getObjectNames()->begin(); jt != (*it)->getObjectNames()->end(); jt++){
            task->addObject(typeName, (*jt));
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
                if (task->objects.find(paramName) == task->objects.end())
                    SystemUtils::abort("Unknown object: " + paramName);

                // std::cout << "Added non fluent (from non fluents): " << paramName << " of type " << task->objects[paramName]->type->name << std::endl;
                params.push_back(task->objects[paramName]); // TODO: this is wrong. It only covers the case that parametrized variable is a Parameter Expression
            }
        }

        if (task->variableDefinitions.find(name) == task->variableDefinitions.end())
            SystemUtils::abort("Variable " + name + " used but not defined.");

        ParametrizedVariable* parent = task->variableDefinitions[name];
        double initialVal = (*it)->getInitValue();

        task->addParametrizedVariable(parent, params, initialVal);

        // std::cout << "###### Added parametrized variable (from addNonFluent) " << name << " with parameters: " << std::endl;
        // for (unsigned i = 0; i < params.size(); i++)
        //     std::cout << "\t" << params[i]->name << std::endl;
        // std::cout << "and value: " << initialVal << std::endl;
    }
    // std::cout << "NonFluents added." << std::endl;
}

void RDDLBlock::addInstance(Instance* instance) {
    // std::cout << "Adding instance" << std::endl;
    this->task->name = instance->getName();

    // Check domain name
    if (this->getDomainName() != instance->getDomainName())
        SystemUtils::abort("Unknown domain " + instance->getDomainName() + " defined in Instance section");

    // Check Non fluents name
    if (this->getNonFluentsName() != instance->getNonFluentsName())
        SystemUtils::abort("Unknown non fluents " + instance->getNonFluentsName() + "defined in Non fluent section");

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
                    if (task->objects.find(paramName) == task->objects.end())
                        SystemUtils::abort("Unknown object: " + paramName);

                    params.push_back(task->objects[paramName]); // TODO: this is wrong. It only covers the case that parametrized variable is and Parameter Expression
                }

            }

            if (task->variableDefinitions.find(name) == task->variableDefinitions.end())
                SystemUtils::abort("Variable " + name + " used but not defined.");

            ParametrizedVariable* parent = task->variableDefinitions[name];
            double initialVal = (*it)->getInitValue();

            task->addParametrizedVariable(parent, params, initialVal);

            // std::cout << "###### Added parametrized (from addInstance) " << name << " with parameters: " << std::endl;
            // for (unsigned i = 0; i < params.size(); i++)
            //     std::cout << "\t" << params[i]->name << std::endl;
            // std::cout << "and value: " << initialVal << std::endl;

        }
    }

    // Set Max nondef actions
    task->numberOfConcurrentActions = instance->getMaxNonDefActions();

    // Set horizon
    task->horizon = instance->getHorizon();

    // Set discount
    task->discountFactor = instance->getDiscount();

    // std::cout << "Instance added." << std::endl;
}

void RDDLBlock::execute(std::string td/*target dir*/) {
    std::cout << "Executing..." << std::endl << "Writing output.." << std::endl;

    Timer t, totalTime;
    std::cout << "Parsing...finished" << std::endl;

    t.reset();
    std::cout << "instantiating..." << std::endl;
    Instantiator instantiator(task);
    instantiator.instantiate();
    std::cout << "...finished (" << t << ")." << std::endl;

    t.reset();
    std::cout << "preprocessing..." << std::endl;
    Preprocessor preprocessor(task);
    preprocessor.preprocess();
    std::cout << "...finished (" << t << ")." << std::endl;

    t.reset();
    std::cout << "analyzing task..." << std::endl;
    TaskAnalyzer analyzer(task);
    analyzer.analyzeTask();
    std::cout << "...finished (" << t << ")." << std::endl;

    t.reset();
    std::cout << "writing output..." << std::endl;
    std::ofstream resultFile;
    std::cout << "Task name " << task->name << std::endl;
    std::string targetDir = td + "/" + task->name;
    resultFile.open(targetDir.c_str());
    task->print(resultFile);
    resultFile.close();
    // task->print(std::cout);
    std::cout << "...finished (" << t << ")." << task->name << std::endl;

    std::cout << "total time: " << totalTime << std::endl;
}