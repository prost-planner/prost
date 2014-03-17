#include "unprocessed_planning_task.h"

#include "functions.h"

#include "utils/string_utils.h"
#include "utils/math_utils.h"
#include "utils/system_utils.h"

#include <iostream>

using namespace std;

UnprocessedPlanningTask::UnprocessedPlanningTask(string _domainDesc, string _problemDesc) :
    domainDesc(_domainDesc),
    numberOfConcurrentActions(numeric_limits<int>::max()),
    horizon(1),
    discountFactor(1.0), 
    rewardCPF(NULL) {

    preprocessInput(_problemDesc);

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

void UnprocessedPlanningTask::preprocessInput(string& problemDesc) {
    StringUtils::standardizeParens(domainDesc);
    StringUtils::standardizeSemicolons(domainDesc);
    StringUtils::standardizeColons(domainDesc);
    StringUtils::standardizeEqualSign(domainDesc);
    StringUtils::removeConsecutiveWhiteSpaces(domainDesc);

    StringUtils::standardizeParens(problemDesc);
    StringUtils::standardizeSemicolons(problemDesc);
    StringUtils::standardizeColons(problemDesc);
    StringUtils::standardizeEqualSign(problemDesc);
    StringUtils::removeConsecutiveWhiteSpaces(problemDesc);

    std::vector<string> nonFluentsAndInstance;
    StringUtils::tokenize(problemDesc, nonFluentsAndInstance);
    if(nonFluentsAndInstance.size() == 1) {
        // There is no non-fluents block
        if(nonFluentsAndInstance[0].find("instance ") != 0) {
            SystemUtils::abort("Error: No instance description found.");
        }

        instanceDesc = nonFluentsAndInstance[0];
    } else {
        assert(nonFluentsAndInstance.size() == 2);
        assert(nonFluentsAndInstance[0].find("non-fluents ") == 0 && nonFluentsAndInstance[1].find("instance ") == 0);

        nonFluentsDesc = nonFluentsAndInstance[0];
        instanceDesc = nonFluentsAndInstance[1];
    }
}

void UnprocessedPlanningTask::addType(string const& name, string const& superType) {
    if(types.find(name) != types.end()) {
        SystemUtils::abort("Error: Type " + name + " is ambiguous.");
    }

    if(superType.empty()) {
        types[name] = new Type(name);
    } else if(types.find(superType) == types.end()) {
        SystemUtils::abort("Error: Supertype not found: " + superType);
    } else {
        types[name] = new Type(name, types[superType]);
    }
}

void UnprocessedPlanningTask::addObject(string const& typeName, string const& objectName) {
    if(types.find(typeName) == types.end()) {
        SystemUtils::abort("Error: Type " + typeName + " not defined.");
    }

    if(objects.find(objectName) != objects.end()) {
        SystemUtils::abort("Error: Object name " + objectName + " is ambiguous.");
    }

    Type* type = types[typeName];

    Object* object = new Object(objectName, type);
    objects[objectName] = object;

    do {
        object->types.push_back(type);
        object->values.push_back(type->objects.size());
        type->objects.push_back(object);
        type = type->superType;
    } while(type);
}

void UnprocessedPlanningTask::addVariableDefinition(ParametrizedVariable* varDef) {
    if(variableDefinitions.find(varDef->fullName) != variableDefinitions.end()) {
        SystemUtils::abort("Error: Ambiguous variable name: " + varDef->fullName);
    }
    variableDefinitions[varDef->fullName] = varDef;
}

void UnprocessedPlanningTask::addParametrizedVariable(ParametrizedVariable* parent, vector<Parameter*> const& params) {
    addParametrizedVariable(parent, params, parent->initialValue);
}

void UnprocessedPlanningTask::addParametrizedVariable(ParametrizedVariable* parent, vector<Parameter*> const& params, double initialValue) {
    if(variableDefinitions.find(parent->variableName) == variableDefinitions.end()) {
        SystemUtils::abort("Error: maldefined parametrized variable.");
    }

    // We declare these here as we need parentheses in the switch otherwise
    StateFluent* sf;
    ActionFluent* af;
    NonFluent* nf;

    switch(parent->variableType) {
    case ParametrizedVariable::STATE_FLUENT:
        sf = new StateFluent(*parent, params, initialValue);
        if(stateFluents.find(sf->fullName) == stateFluents.end()) {
            stateFluents[sf->fullName] = sf;
            if(variablesBySchema.find(parent) == variablesBySchema.end()) {
                variablesBySchema[parent] = vector<StateFluent*>();
            }
            variablesBySchema[parent].push_back(sf);
        }
        break;
    case ParametrizedVariable::ACTION_FLUENT:
        af = new ActionFluent(*parent, params);
        assert(actionFluents.find(af->fullName) == actionFluents.end());
        actionFluents[af->fullName] = af;
        break;
    case ParametrizedVariable::NON_FLUENT:
        nf = new NonFluent(*parent, params, initialValue);
        if(nonFluents.find(nf->fullName) == nonFluents.end()) {
            nonFluents[nf->fullName] = nf;
        }
        break;
        // case ParametrizedVariable::INTERM_FLUENT:
        // assert(false);
        // break;
    }    
}

StateFluent* UnprocessedPlanningTask::getStateFluent(std::string const& name) {
    if(stateFluents.find(name) == stateFluents.end()) {
        SystemUtils::abort("Error: state-fluent " + name + " used but not defined.");
    }
    return stateFluents[name];
}

ActionFluent* UnprocessedPlanningTask::getActionFluent(std::string const& name) {
    if(actionFluents.find(name) == actionFluents.end()) {
        SystemUtils::abort("Error: action-fluent " + name + " used but not defined.");
    }
    return actionFluents[name];
}

NonFluent* UnprocessedPlanningTask::getNonFluent(std::string const& name) {
    if(nonFluents.find(name) == nonFluents.end()) {
        SystemUtils::abort("Error: non-fluent " + name + " used but not defined.");
    }
    return nonFluents[name];
}

vector<StateFluent*> UnprocessedPlanningTask::getVariablesOfSchema(ParametrizedVariable* schema) {
    assert(variablesBySchema.find(schema) != variablesBySchema.end());
    return variablesBySchema[schema];
}

void UnprocessedPlanningTask::addStateActionConstraint(LogicalExpression* sac) {
    SACs.push_back(sac);
}

void UnprocessedPlanningTask::addCPF(ConditionalProbabilityFunction* const& cpf) {
    for(unsigned int i = 0; i < CPFs.size(); ++i) {
        if(cpf->head->fullName == CPFs[i]->head->fullName) {
            SystemUtils::abort("Error: CPF with same name exists already: " + cpf->head->fullName);
        }
    }
    CPFs.push_back(cpf);
}

void UnprocessedPlanningTask::setRewardCPF(LogicalExpression* const& rewardFormula) {
    if(rewardCPF) {
        SystemUtils::abort("Error: RewardCPF exists already.");
    }
    rewardCPF = new RewardFunction(rewardFormula);
}
