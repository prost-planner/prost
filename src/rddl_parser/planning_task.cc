#include "planning_task.h"

#include "evaluatables.h"

#include "utils/string_utils.h"
#include "utils/math_utils.h"
#include "utils/system_utils.h"

#include <iostream>

using namespace std;

PlanningTask::PlanningTask() :
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

void PlanningTask::addType(string const& name, string const& superType) {
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

void PlanningTask::addObject(string const& typeName, string const& objectName) {
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

void PlanningTask::addVariableDefinition(ParametrizedVariable* varDef) {
    if(variableDefinitions.find(varDef->fullName) != variableDefinitions.end()) {
        SystemUtils::abort("Error: Ambiguous variable name: " + varDef->fullName);
    }
    variableDefinitions[varDef->fullName] = varDef;
}

void PlanningTask::addParametrizedVariable(ParametrizedVariable* parent, vector<Parameter*> const& params) {
    addParametrizedVariable(parent, params, parent->initialValue);
}

void PlanningTask::addParametrizedVariable(ParametrizedVariable* parent, vector<Parameter*> const& params, double initialValue) {
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

StateFluent* PlanningTask::getStateFluent(string const& name) {
    for(unsigned int i = 0; i < stateFluents.size(); ++i) {
        if(name == stateFluents[i]->fullName) {
            return stateFluents[i];
        }
    }
    SystemUtils::abort("Error: state-fluent " + name + " used but not defined.");
    return NULL;
}

ActionFluent* PlanningTask::getActionFluent(string const& name) {
    for(unsigned int i = 0; i < actionFluents.size(); ++i) {
        if(name == actionFluents[i]->fullName) {
            return actionFluents[i];
        }
    }
    SystemUtils::abort("Error: action-fluent " + name + " used but not defined.");
    return NULL;
}

NonFluent* PlanningTask::getNonFluent(string const& name) {
    for(unsigned int i = 0; i < nonFluents.size(); ++i) {
        if(name == nonFluents[i]->fullName) {
            return nonFluents[i];
        }
    }
    SystemUtils::abort("Error: non-fluent " + name + " used but not defined.");
    return NULL;
}

vector<StateFluent*> PlanningTask::getVariablesOfSchema(ParametrizedVariable* schema) {
    assert(variablesBySchema.find(schema) != variablesBySchema.end());
    return variablesBySchema[schema];
}

void PlanningTask::addStateActionConstraint(LogicalExpression* sac) {
    SACs.push_back(sac);
}

void PlanningTask::addCPF(ConditionalProbabilityFunction* const& cpf) {
    for(unsigned int i = 0; i < CPFs.size(); ++i) {
        if(cpf->head->fullName == CPFs[i]->head->fullName) {
            SystemUtils::abort("Error: CPF with same name exists already: " + cpf->head->fullName);
        }
    }
    CPFs.push_back(cpf);
}

void PlanningTask::setRewardCPF(LogicalExpression* const& rewardFormula) {
    if(rewardCPF) {
        SystemUtils::abort("Error: RewardCPF exists already.");
    }
    rewardCPF = new RewardFunction(rewardFormula);
}

void PlanningTask::print(ostream& out) {
    int firstProbabilisticVarIndex = (int)CPFs.size();
    bool deterministic = true;
    for(unsigned int i = 0; i < CPFs.size(); ++i) {
        if(CPFs[i]->isProbabilistic()) {
            firstProbabilisticVarIndex = i;
            deterministic = false;
            break;
        }
    }

    out << "-----TASK-----" << endl;
    out << "instance name : " << name << endl;
    out << "horizon : " << horizon << endl;
    out << "discount factor : " << discountFactor << endl;
    out << "number of action fluents : " << actionFluents.size() << endl;
    out << "number of state fluents : " << CPFs.size() << endl;
    out << "number of preconds : " << dynamicSACs.size() << endl;
    out << "number of actions : " << actionStates.size() << endl;
    out << "number of deterministic state fluents : " << firstProbabilisticVarIndex << endl;
    out << "number of hashing functions : " << (dynamicSACs.size() + CPFs.size() + 1) << endl;
    out << "initial state : ";
    for(unsigned int i = 0; i < CPFs.size(); ++i) {
        out << CPFs[i]->getInitialValue() << " ";
    }
    out << endl;
    out << "deterministic : " << deterministic << endl;
    out << "state hashing possible : " << !stateHashKeys.empty() << endl;
    out << "kleene state hashing possible : " << !kleeneStateHashKeyBases.empty() << endl;
    out << "noop is optimal final action : " << noopIsOptimalFinalAction << endl;
    out << "reward formula allows reward lock detection : " << rewardFormulaAllowsRewardLockDetection << endl;
    out << endl << endl;

    out << "-----FLUENTS-----" << endl;
    for(unsigned int i = 0; i < actionFluents.size(); ++i) {
        out << "action-fluent " << actionFluents[i]->index << endl;
        out << "name : " << actionFluents[i]->fullName << endl;
        out << "values : 0 : false, 1 : true" << endl; // TODO: issue-14
        out << "-----" << endl;
    }

    for(unsigned int index = 0; index < CPFs.size(); ++index) {
        assert(CPFs[index]->head->index == index);
        out << "state-fluent " << index << endl;
        out << "name : " << CPFs[index]->head->fullName << endl;
        out << "values : ";
        for(set<double>::iterator it = CPFs[index]->domain.begin(); it != CPFs[index]->domain.end();) {
            out << *it << " : " << CPFs[index]->head->valueType->objects[*it]->name;
            ++it;
            
            if(it != CPFs[index]->domain.end()) {
                out << ", ";
            } else {
                out << endl;
            }
        }
        out << "hash index : " << CPFs[index]->hashIndex << endl;
        out << "caching type : " << CPFs[index]->cachingType << endl;
        if(CPFs[index]->cachingType == "VECTOR") {
            out << "caching vec size : " << CPFs[index]->cachingVectorSize << endl;
        }
        out << "kleene caching type : " << CPFs[index]->kleeneCachingType << endl;
        if(CPFs[index]->kleeneCachingType == "VECTOR") {
            out << "kleene caching vec size : " << CPFs[index]->kleeneCachingVectorSize << endl;
        }
        out << "-----" << endl;
    }

    for(unsigned int index = 0; index < dynamicSACs.size(); ++index) {
        assert(dynamicSACs[index]->index == index);
        out << "precond " << index << endl;
        out << "hash index : " << dynamicSACs[index]->hashIndex << endl;
        out << "caching type : " << dynamicSACs[index]->cachingType << endl;
        if(dynamicSACs[index]->cachingType == "VECTOR") {
            out << "caching vec size : " << dynamicSACs[index]->cachingVectorSize << endl;
        }
        out << "kleene caching type : " << dynamicSACs[index]->kleeneCachingType << endl;
        if(dynamicSACs[index]->kleeneCachingType == "VECTOR") {
            out << "kleene caching vec size : " << dynamicSACs[index]->kleeneCachingVectorSize << endl;
        }
        out << "-----" << endl;
    }

    out << "reward" << endl;
    out << "min : " << *rewardCPF->domain.begin() << endl;
    out << "max : " << *rewardCPF->domain.rbegin() << endl;
    out << "action independent : " << rewardCPF->isActionIndependent() << endl;
    out << "hash index : " << rewardCPF->hashIndex << endl;
    out << "caching type : " << rewardCPF->cachingType << endl;
    if(rewardCPF->cachingType == "VECTOR") {
        out << "caching vec size : " << rewardCPF->cachingVectorSize << endl;
    }
    out << "kleene caching type : " << rewardCPF->kleeneCachingType << endl;
    if(rewardCPF->kleeneCachingType == "VECTOR") {
        out << "kleene caching vec size : " << rewardCPF->kleeneCachingVectorSize << endl;
    }
    // for(set<double>::iterator it = rewardCPF->domain.begin(); it != rewardCPF->domain.end();) {
    //     out << *it;
    //     ++it;

    //     if(it != rewardCPF->domain.end()) {
    //         out << ", ";
    //     } else {
    //         out << endl << endl;
    //     }
    // }

    out << endl << "-----EVALUATABLES-----" << endl;
    for(unsigned int index = 0; index < CPFs.size(); ++index) {
        assert(CPFs[index]->head->index == index);
        out << "state-fluent " << index << endl;
        out << "orig : ";
        CPFs[index]->formula->print(out);
        out << endl;
        if(CPFs[index]->isProbabilistic()) {
            out << "det : ";
            CPFs[index]->determinization->print(out);
            out << endl;
        }
        out << "-----" << endl;
    }

    out << "reward" << endl;
    rewardCPF->formula->print(out);
    out << endl;
    out << "-----" << endl;

    for(unsigned int index = 0; index < dynamicSACs.size(); ++index) {
        assert(dynamicSACs[index]->index == index);
        out << "precond " << index << endl;
        dynamicSACs[index]->formula->print(out);
        out << endl;
        out << "-----" << endl;
    }

    out << endl;

    out << "-----ACTION STATES-----" << endl;
    for(unsigned int index = 0; index < actionStates.size(); ++index) {
        out << "action-state " << index << endl;
        out << "fluent values : ";
        for(unsigned int varIndex = 0; varIndex < actionStates[index].state.size(); ++varIndex) {
            out << actionStates[index][varIndex] << " ";
        }
        out << endl;
        out << "preconds : ";
        for(unsigned int sacIndex = 0; sacIndex < actionStates[index].relevantSACs.size(); ++sacIndex) {
            out << actionStates[index].relevantSACs[sacIndex]->index << " ";
        }
        out << endl;
        out << "-----" << endl;
    }
    out << endl;

    out << "-----HASH KEYS-----" << endl;
    for(unsigned int index = 0; index < CPFs.size(); ++index) {
        assert(CPFs[index]->head->index == index);
        out << "state-fluent " << index << endl;
        if(!stateHashKeys.empty()) {
            out << "state hash key : ";
            for(unsigned int valIndex = 0; valIndex < stateHashKeys[index].size(); ++valIndex) {
                out << stateHashKeys[index][valIndex];
                if(valIndex != stateHashKeys[index].size() - 1) {
                    out << ", ";
                }
            }
        }
        if(!kleeneStateHashKeyBases.empty()) {
            out << endl << "kleene state hash key base : " << kleeneStateHashKeyBases[index] << endl;
        } else {
            out << endl;
        }
        out << "state fluent hash keys : " << indexToStateFluentHashKeyMap[index].size() << endl;
        for(unsigned int i = 0; i < indexToStateFluentHashKeyMap[index].size(); ++i) {
            out << indexToStateFluentHashKeyMap[index][i].first << " : ";
            out << indexToStateFluentHashKeyMap[index][i].second << endl;
            // for(unsigned int j = 0; j < CPFs[index]->domain.size(); ++j) {
            //     out << (j * indexToStateFluentHashKeyMap[index][i].second);
            //     if(j != CPFs[index]->domain.size() - 1) {
            //         out << ", ";
            //     }
            // }
            // out << endl;
        }
        out << "kleene state fluent hash keys : " << indexToKleeneStateFluentHashKeyMap[index].size() << endl;
        for(unsigned int i = 0; i < indexToKleeneStateFluentHashKeyMap[index].size(); ++i) {
            out << indexToKleeneStateFluentHashKeyMap[index][i].first << " : " << indexToKleeneStateFluentHashKeyMap[index][i].second << endl;
        }

        out << "-----" << endl;
    }

    out << endl << "ACTION HASH KEYS" << endl;
    for(unsigned int index = 0; index < CPFs.size(); ++index) {
        assert(CPFs[index]->head->index == index);
        out << "state-fluent " << index << endl;
        for(unsigned int actionIndex = 0; actionIndex < CPFs[index]->actionHashKeyMap.size(); ++actionIndex) {
            out << actionIndex << " : " << CPFs[index]->actionHashKeyMap[actionIndex];
            if(actionIndex != CPFs[index]->actionHashKeyMap.size() - 1) {
                out << ", ";
            }
        }
        out << endl;
        out << "-----" << endl;
    }

    for(unsigned int index = 0; index < dynamicSACs.size(); ++index) {
        assert(dynamicSACs[index]->index == index);
        out << "precond " << index << endl;
        for(unsigned int actionIndex = 0; actionIndex < dynamicSACs[index]->actionHashKeyMap.size(); ++actionIndex) {
            out << actionIndex << " : " << dynamicSACs[index]->actionHashKeyMap[actionIndex];
            if(actionIndex != dynamicSACs[index]->actionHashKeyMap.size() - 1) {
                out << ", ";
            }
        }
        out << endl;
        out << "-----" << endl;
    }

    out << "reward" << endl;
    for(unsigned int actionIndex = 0; actionIndex < rewardCPF->actionHashKeyMap.size(); ++actionIndex) {
        out << actionIndex << " : " << rewardCPF->actionHashKeyMap[actionIndex];
        if(actionIndex != rewardCPF->actionHashKeyMap.size() - 1) {
            out << ", ";
        }
    }
    out << endl;
    out << "-----" << endl;

    out << endl << "TRAINING SET" << endl;
    for(set<State>::iterator it = trainingSet.begin(); it != trainingSet.end(); ++it) {
        for(unsigned int i = 0; i < it->state.size(); ++i) {
            out << it->state[i] << " ";
        }
        out << endl;
    }
}
