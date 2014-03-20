#include "unprocessed_planning_task.h"

#include "evaluatables.h"

#include "utils/string_utils.h"
#include "utils/math_utils.h"
#include "utils/system_utils.h"

#include <iostream>

using namespace std;

UnprocessedPlanningTask::UnprocessedPlanningTask() :
    numberOfConcurrentActions(numeric_limits<int>::max()),
    horizon(1),
    discountFactor(1.0), 
    rewardCPF(NULL) {

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
        SystemUtils::abort("Error: Parametrized variable " + parent->variableName + " not defined.");
    }

    // We declare these here as we need parentheses in the switch otherwise
    StateFluent* sf;
    ActionFluent* af;
    NonFluent* nf;

    switch(parent->variableType) {
    case ParametrizedVariable::STATE_FLUENT:
        sf = new StateFluent(*parent, params, initialValue);
        for(unsigned int i = 0; i < stateFluents.size(); ++i) {
            // This could already be defined if it occurs in the initial state
            if(sf->fullName == stateFluents[i]->fullName) {
                return;
            }
        }
        stateFluents.push_back(sf);

        if(variablesBySchema.find(parent) == variablesBySchema.end()) {
            variablesBySchema[parent] = vector<StateFluent*>();
        }
        variablesBySchema[parent].push_back(sf);
        break;
    case ParametrizedVariable::ACTION_FLUENT:
        af = new ActionFluent(*parent, params);
        for(unsigned int i = 0; i < actionFluents.size(); ++i) {
            assert(af->fullName != actionFluents[i]->fullName);
        }
        actionFluents.push_back(af);
        break;
    case ParametrizedVariable::NON_FLUENT:
        nf = new NonFluent(*parent, params, initialValue);
        for(unsigned int i = 0; i < nonFluents.size(); ++i) {
            // This mightbe defined if it occurs in the non fluents entry
            if(nf->fullName == nonFluents[i]->fullName) {
                return;
            }
        }
        nonFluents.push_back(nf);
        // case ParametrizedVariable::INTERM_FLUENT:
        // assert(false);
        // break;
    }    
}

StateFluent* UnprocessedPlanningTask::getStateFluent(string const& name) {
    for(unsigned int i = 0; i < stateFluents.size(); ++i) {
        if(name == stateFluents[i]->fullName) {
            return stateFluents[i];
        }
    }
    SystemUtils::abort("Error: state-fluent " + name + " used but not defined.");
    return NULL;
}

ActionFluent* UnprocessedPlanningTask::getActionFluent(string const& name) {
    for(unsigned int i = 0; i < actionFluents.size(); ++i) {
        if(name == actionFluents[i]->fullName) {
            return actionFluents[i];
        }
    }
    SystemUtils::abort("Error: action-fluent " + name + " used but not defined.");
    return NULL;
}

NonFluent* UnprocessedPlanningTask::getNonFluent(string const& name) {
    for(unsigned int i = 0; i < nonFluents.size(); ++i) {
        if(name == nonFluents[i]->fullName) {
            return nonFluents[i];
        }
    }
    SystemUtils::abort("Error: non-fluent " + name + " used but not defined.");
    return NULL;
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
