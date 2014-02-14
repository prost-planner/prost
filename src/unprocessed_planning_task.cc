#include "unprocessed_planning_task.h"

#include "typed_objects.h"
#include "logical_expressions.h"
#include "actions.h"
#include "conditional_probability_function.h"

#include "utils/string_utils.h"
#include "utils/math_utils.h"
#include "utils/system_utils.h"

#include <iostream>

using namespace std;

UnprocessedPlanningTask::UnprocessedPlanningTask(string _domainDesc, string _problemDesc) :
    domainDesc(_domainDesc), numberOfConcurrentActions (1), horizon (1), discountFactor(1.0), 
    rewardVariableDefinition(NULL), rewardVariable(NULL), rewardCPF(NULL) {

    preprocessInput(_problemDesc);

    requirements["continuous"] = false;
    requirements["multivalued"] = false;
    requirements["reward-deterministic"] = false;
    requirements["intermediate-nodes"] = false;
    requirements["constrained-state"] = false;
    requirements["partially-observed"] = false;
    requirements["concurrent"] = false;
    requirements["integer-valued"] = false;
    requirements["cpf-deterministic"] = false;

    addObjectType(ObjectType::objectRootInstance());
    addObjectType(ObjectType::enumRootInstance());

    constants[NumericConstant::falsity()->value] = NumericConstant::falsity();
    constants[NumericConstant::truth()->value] = NumericConstant::truth();

    addVariableDefinition(VariableDefinition::rewardInstance());
    addStateFluent(StateFluent::rewardInstance());
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
        assert(nonFluentsAndInstance[0].find("instance ") == 0);

        instanceDesc = nonFluentsAndInstance[0];
    } else {
        assert(nonFluentsAndInstance.size() == 2);
        assert(nonFluentsAndInstance[0].find("non-fluents ") == 0 && nonFluentsAndInstance[1].find("instance ") == 0);

        nonFluentsDesc = nonFluentsAndInstance[0];
        instanceDesc = nonFluentsAndInstance[1];
    }
}

void UnprocessedPlanningTask::addObjectType(ObjectType* objectType) {
    assert(objectTypes.find(objectType->name) == objectTypes.end());
    objectTypes[objectType->name] = objectType;
    assert(objectsByType.find(objectType) == objectsByType.end());
    objectsByType[objectType] = vector<Object*>();
}

ObjectType* UnprocessedPlanningTask::getObjectType(string& name) {
    assert(objectTypes.find(name) != objectTypes.end());
    return objectTypes[name];
}

void UnprocessedPlanningTask::addObject(Object* object) {
    assert(objects.find(object->name) == objects.end());
    objects[object->name] = object;
    vector<ObjectType*> types;
    object->getObjectTypes(types);
    for(unsigned int i = 0; i < types.size(); ++i) {
        assert(objectsByType.find(types[i]) != objectsByType.end());
        objectsByType[types[i]].push_back(object);
    }
}

Object* UnprocessedPlanningTask::getObject(string& name) {
    assert(objects.find(name) != objects.end());
    return objects[name];
}

void UnprocessedPlanningTask::getObjectsOfType(ObjectType* objectType, std::vector<Object*>& res) {
    assert(objectsByType.find(objectType) != objectsByType.end());
    res = objectsByType[objectType];
}

void UnprocessedPlanningTask::addVariableDefinition(VariableDefinition* varDef) {
    if(varDef == VariableDefinition::rewardInstance()) {
        assert(!rewardVariableDefinition);
        rewardVariableDefinition = varDef;
    } else {
        variableDefinitionsVec.push_back(varDef);
    }
    assert(variableDefinitions.find(varDef->name) == variableDefinitions.end());
    variableDefinitions[varDef->name] = varDef;
}

VariableDefinition* UnprocessedPlanningTask::getVariableDefinition(std::string& name) {
    assert(variableDefinitions.find(name) != variableDefinitions.end());
    return variableDefinitions[name];
}

void UnprocessedPlanningTask::getVariableDefinitions(vector<VariableDefinition*>& result, bool includeRewardDef) {
    result = variableDefinitionsVec;
    if(includeRewardDef) {
        result.push_back(rewardVariableDefinition);
    }
}

bool UnprocessedPlanningTask::isAVariableDefinition(string& name) {
    return (variableDefinitions.find(name) != variableDefinitions.end());
}

void UnprocessedPlanningTask::addCPFDefinition(pair<UninstantiatedVariable*, LogicalExpression*> const& CPFDef) {
    for(unsigned int i = 0; i < CPFDefs.size(); ++i) {
        if(CPFDefs[i].first->parent->name == CPFDef.first->parent->name) {
            SystemUtils::abort("Error: Ambiguous definition of CPF: " + CPFDef.first->parent->name);
        }
    }
    CPFDefs.push_back(CPFDef);
}

void UnprocessedPlanningTask::addStateActionConstraint(LogicalExpression* sac) {
    SACs.push_back(sac);
}

void UnprocessedPlanningTask::removeStateActionConstraint(LogicalExpression* sac) {
    for(vector<LogicalExpression*>::iterator it = SACs.begin(); it != SACs.end(); ++it) {
        if(*it == sac) {
            SACs.erase(it);
            break;
        }
    }
}

void UnprocessedPlanningTask::addStateFluent(StateFluent* var) {
    if(var == StateFluent::rewardInstance()) {
        assert(!rewardVariable);
        rewardVariable = var;
    }

    size_t cutPos = var->name.find("(");
    assert(cutPos != string::npos);

    string parentName = var->name.substr(0,cutPos);
    assert(variableDefinitions.find(parentName) != variableDefinitions.end());
    VariableDefinition* parent = variableDefinitions[parentName];
    assert(parent->variableType == VariableDefinition::STATE_FLUENT || parent->variableType == VariableDefinition::INTERM_FLUENT);

    if(variables.find(var->name) == variables.end()) {
        variables[var->name] = var;
        if(var != StateFluent::rewardInstance()) {
            if(variablesBySchema.find(parent) == variablesBySchema.end()) {
                variablesBySchema[parent] = vector<AtomicLogicalExpression*>();
            }
            variablesBySchema[parent].push_back(var);
        }
    }
}

StateFluent* UnprocessedPlanningTask::getStateFluent(UninstantiatedVariable* var) {
    assert(variables.find(var->name) != variables.end());
    return variables[var->name];
}

void UnprocessedPlanningTask::addActionFluent(ActionFluent* var) {
    size_t cutPos = var->name.find("(");
    assert(cutPos != string::npos);

    string parentName = var->name.substr(0,cutPos);
    assert(variableDefinitions.find(parentName) != variableDefinitions.end());
    VariableDefinition* parent = variableDefinitions[parentName];
    assert(parent->variableType == VariableDefinition::ACTION_FLUENT);

    assert(actions.find(var->name) == actions.end());
    actions[var->name] = var;
    if(variablesBySchema.find(parent) == variablesBySchema.end()) {
        variablesBySchema[parent] = vector<AtomicLogicalExpression*>();
    }
    variablesBySchema[parent].push_back(var);
}

ActionFluent* UnprocessedPlanningTask::getActionFluent(UninstantiatedVariable* var) {
    assert(actions.find(var->name) != actions.end());
    return actions[var->name];
}

void UnprocessedPlanningTask::getActionFluents(vector<ActionFluent*>& result) {
    for(map<string, ActionFluent*>::iterator it = actions.begin(); it != actions.end(); ++it) {
        result.push_back(it->second);
    }
}

void UnprocessedPlanningTask::addNonFluent(NonFluent* var) {
    size_t cutPos = var->name.find("(");
    assert(cutPos != string::npos);

    string parentName = var->name.substr(0,cutPos);
    assert(variableDefinitions.find(parentName) != variableDefinitions.end());
    VariableDefinition* parent = variableDefinitions[parentName];
    assert(parent->variableType == VariableDefinition::NON_FLUENT);

    if(nonFluents.find(var->name) == nonFluents.end()) {
        nonFluents[var->name] = var;
        if(variablesBySchema.find(parent) == variablesBySchema.end()) {
            variablesBySchema[parent] = vector<AtomicLogicalExpression*>();
        }
        variablesBySchema[parent].push_back(var);
    }
}

NonFluent* UnprocessedPlanningTask::getNonFluent(UninstantiatedVariable* var) {
    assert(nonFluents.find(var->name) != nonFluents.end());
    return nonFluents[var->name];
}

void UnprocessedPlanningTask::getVariablesOfSchema(VariableDefinition* schema, std::vector<AtomicLogicalExpression*>& result) {
    if(schema == VariableDefinition::rewardInstance()) {
        assert(rewardVariable);
        result.push_back(rewardVariable);
    } else {
        assert(variablesBySchema.find(schema) != variablesBySchema.end());
        result = variablesBySchema[schema];
    }
}

void UnprocessedPlanningTask::getInitialState(std::vector<AtomicLogicalExpression*>& result) {
    for(map<VariableDefinition*, vector<AtomicLogicalExpression*> >::iterator it = variablesBySchema.begin(); it != variablesBySchema.end(); ++it) {
        if(it->first->variableType != VariableDefinition::ACTION_FLUENT) {
            for(unsigned int i = 0; i < it->second.size(); ++i) {
                result.push_back(it->second[i]);
            }
        }
    }
}

void UnprocessedPlanningTask::addCPF(std::pair<StateFluent*, LogicalExpression*> const& cpf) {
    for(unsigned int i = 0; i < CPFs.size(); ++i) {
        if(cpf.first->name == CPFs[i].first->name) {
            SystemUtils::abort("Error: CPF with same name exists already: " + cpf.first->name);
        }
    }
    CPFs.push_back(cpf);
}

void UnprocessedPlanningTask::setRewardCPF(LogicalExpression* const& _rewardCPF) {
    if(rewardCPF) {
        SystemUtils::abort("Error: RewardCPF exists already.");
    }
    rewardCPF = _rewardCPF;
}

// void UnprocessedPlanningTask::getCPFs(vector<ConditionalProbabilityFunction*>& result, bool includeRewardCPF) {
//     result = CPFs;
//     if(includeRewardCPF) {
//         result.push_back(rewardCPF);
//     }
// }

// void UnprocessedPlanningTask::removeCPF(ConditionalProbabilityFunction* cpf) {
//     assert(!cpf->isRewardCPF());
//     for(vector<ConditionalProbabilityFunction*>::iterator it = CPFs.begin(); it != CPFs.end(); ++it) {
//         if(*it == cpf) {
//             CPFs.erase(it);
//             break;
//         }
//     }
// }
