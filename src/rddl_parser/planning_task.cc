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
    rewardCPF(NULL),
    rewardLockDetected(false),
    unreasonableActionDetected(false),
    unreasonableActionInDeterminizationDetected(false),
    nonTerminalStatesWithUniqueAction(0) {
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
    // Set precision of doubles
    out.unsetf(ios::floatfield);
    out.precision(numeric_limits<double>::digits10);

    int firstProbabilisticVarIndex = (int)CPFs.size();
    bool deterministic = true;
    for(unsigned int i = 0; i < CPFs.size(); ++i) {
        if(CPFs[i]->isProbabilistic()) {
            firstProbabilisticVarIndex = i;
            deterministic = false;
            break;
        }
    }

    out << "#####TASK#####" << endl;
    out << "## name" << endl;
    out << name << endl;
    out << "## horizon" << endl;
    out << horizon << endl;
    out << "## discount factor" << endl;
    out << discountFactor << endl;
    out << "## number of action fluents" << endl;
    out << actionFluents.size() << endl;
    out << "## number of det state fluents" << endl;
    out << firstProbabilisticVarIndex << endl;
    out << "## number of prob state fluents" << endl;
    out << (CPFs.size() - firstProbabilisticVarIndex) << endl;
    out << "## number of preconds" << endl;
    out << dynamicSACs.size() << endl;
    out << "## number of actions" << endl;
    out << actionStates.size() << endl;
    out << "## number of hashing functions" << endl;
    out << (dynamicSACs.size() + CPFs.size() + 1) << endl;
    out << "## initial state" << endl;
    for(unsigned int i = 0; i < CPFs.size(); ++i) {
        out << CPFs[i]->getInitialValue() << " ";
    }
    out << endl;
    out << "## 1 if task is deterministic" << endl;
    out << deterministic << endl;
    out << "## 1 if state hashing possible"  << endl;
    out << !stateHashKeys.empty() << endl;
    out << "## 1 if kleene state hashing possible" << endl;
    out << !kleeneStateHashKeyBases.empty() << endl;
    out << "## method to calculate the final reward" << endl;
    out << finalRewardCalculationMethod << endl;
    if(finalRewardCalculationMethod == "BEST_OF_CANDIDATE_SET") {
        out << "## set of candidates to calculate final reward (first line is the number)" << endl;
        out << candidatesForOptimalFinalAction.size() << endl;
        for(unsigned int i = 0; i < candidatesForOptimalFinalAction.size(); ++i) {
            out << candidatesForOptimalFinalAction[i] << " ";
        }
        out << endl;
    }
    out << "## 1 if reward formula allows reward lock detection and a reward lock was found during task analysis" << endl;
    out << rewardLockDetected << endl;
    out << "## 1 if an unreasonable action was detected" << endl;
    out << unreasonableActionDetected << endl;
    out << "## 1 if an unreasonable action was detected in the determinization" << endl;
    out << unreasonableActionInDeterminizationDetected << endl;
    out << "## number of states with only one applicable reasonable action that were" << endl;
    out << "## detected during task analysis, and the total number of encountered states" << endl;
    out << nonTerminalStatesWithUniqueAction << " " << numberOfEncounteredStates << endl;

    out << endl << endl << "#####ACTION FLUENTS#####" << endl;
    for(unsigned int i = 0; i < actionFluents.size(); ++i) {
        out << "## index" << endl;
        out << actionFluents[i]->index << endl;
        out << "## name" << endl;
        out << actionFluents[i]->fullName << endl;
        out << "## number of values" << endl;
        out << "2" << endl;
        out << "## values" << endl;
        out << "0 false" << endl << "1 true" << endl;
        out << endl;
    }

    out << endl << endl << "#####DET STATE FLUENTS AND CPFS#####" << endl;
    for(unsigned int index = 0; index < firstProbabilisticVarIndex; ++index) {
        assert(CPFs[index]->head->index == index);
        assert(!CPFs[index]->isProbabilistic());
        out << "## index" << endl;
        out << index << endl;
        out << "## name" << endl;
        out << CPFs[index]->head->fullName << endl;
        out << "## number of values" << endl;
        out << CPFs[index]->domain.size() << endl;
        out << "## values" << endl;
        for(set<double>::iterator it = CPFs[index]->domain.begin(); it != CPFs[index]->domain.end(); ++it) {
            out << *it << " " << CPFs[index]->head->valueType->objects[*it]->name << endl;
        }

        out << "## formula" << endl;
        CPFs[index]->formula->print(out);
        out << endl;

        out << "## hash index" << endl;
        out << CPFs[index]->hashIndex << endl;
        out << "## caching type " << endl;
        out << CPFs[index]->cachingType << endl;
        if(CPFs[index]->cachingType == "VECTOR") {
            out << "## precomputed results" << endl;
            out << CPFs[index]->precomputedResults.size() << endl;
            for(unsigned int res = 0; res < CPFs[index]->precomputedResults.size(); ++res) {
                out << res << " " << CPFs[index]->precomputedResults[res] << endl;
            }
        }
        out << "## kleene caching type" << endl;
        out << CPFs[index]->kleeneCachingType << endl;
        if(CPFs[index]->kleeneCachingType == "VECTOR") {
            out << "## kleene caching vec size" << endl;
            out << CPFs[index]->kleeneCachingVectorSize << endl;
        }

        out << "## action hash keys" << endl;
        for(unsigned int actionIndex = 0; actionIndex < CPFs[index]->actionHashKeyMap.size(); ++actionIndex) {
            out << actionIndex << " " << CPFs[index]->actionHashKeyMap[actionIndex] << endl;
        }
        out << endl;
    }

    out << endl << endl << "#####PROB STATE FLUENTS AND CPFS#####" << endl;
    for(unsigned int index = firstProbabilisticVarIndex; index < CPFs.size(); ++index) {
        assert(CPFs[index]->head->index == index);
        assert(CPFs[index]->isProbabilistic());
        out << "## index" << endl;
        out << (index - firstProbabilisticVarIndex) << endl;
        out << "## name" << endl;
        out << CPFs[index]->head->fullName << endl;
        out << "## number of values" << endl;
        out << CPFs[index]->domain.size() << endl;
        out << "## values" << endl;
        for(set<double>::iterator it = CPFs[index]->domain.begin(); it != CPFs[index]->domain.end(); ++it) {
            out << *it << " " << CPFs[index]->head->valueType->objects[*it]->name << endl;
        }

        out << "## formula" << endl;
        CPFs[index]->formula->print(out);
        out << endl;

        out << "## determinized formula" << endl;
        CPFs[index]->determinization->print(out);
        out << endl;

        out << "## hash index" << endl;
        out << CPFs[index]->hashIndex << endl;
        out << "## caching type " << endl;
        out << CPFs[index]->cachingType << endl;
        if(CPFs[index]->cachingType == "VECTOR") {
            out << "## precomputed results (key - determinization - size of distribution - value-probability pairs)" << endl;
            out << CPFs[index]->precomputedResults.size() << endl;
            for(unsigned int res = 0; res < CPFs[index]->precomputedResults.size(); ++res) {
                out << res << " " << CPFs[index]->precomputedResults[res] << " " << CPFs[index]->precomputedPDResults[res].values.size();
                for(unsigned int valProbPair = 0; valProbPair < CPFs[index]->precomputedPDResults[res].values.size(); ++valProbPair) {
                    out << " " << CPFs[index]->precomputedPDResults[res].values[valProbPair] << " " << CPFs[index]->precomputedPDResults[res].probabilities[valProbPair];
                }
                out << endl;
            }
        }
        out << "## kleene caching type" << endl;
        out << CPFs[index]->kleeneCachingType << endl;
        if(CPFs[index]->kleeneCachingType == "VECTOR") {
            out << "## kleene caching vec size" << endl;
            out << CPFs[index]->kleeneCachingVectorSize << endl;
        }

        out << "## action hash keys" << endl;
        for(unsigned int actionIndex = 0; actionIndex < CPFs[index]->actionHashKeyMap.size(); ++actionIndex) {
            out << actionIndex << " " << CPFs[index]->actionHashKeyMap[actionIndex] << endl;
        }

        out << endl;
    }

    out << endl << endl << "#####REWARD#####" << endl;
    out << "## formula" << endl;
    rewardCPF->formula->print(out);
    out << endl;
    out << "## min" << endl;
    out << *rewardCPF->domain.begin() << endl;
    out << "## max" << endl;
    out << *rewardCPF->domain.rbegin() << endl;
    out << "## hash index" << endl;
    out << rewardCPF->hashIndex << endl;
    out << "## caching type" << endl; 
    out << rewardCPF->cachingType << endl;
    if(rewardCPF->cachingType == "VECTOR") {
        out << "## precomputed results" << endl;
        out << rewardCPF->precomputedResults.size() << endl;
        for(unsigned int res = 0; res < rewardCPF->precomputedResults.size(); ++res) {
            out << res << " " << rewardCPF->precomputedResults[res] << endl;
        }
    }
    out << "## kleene caching type" << endl;
    out << rewardCPF->kleeneCachingType << endl;
    if(rewardCPF->kleeneCachingType == "VECTOR") {
        out << "## kleene caching vec size" << endl;
        out << rewardCPF->kleeneCachingVectorSize << endl;
    }

    out << "## action hash keys" << endl;
    for(unsigned int actionIndex = 0; actionIndex < rewardCPF->actionHashKeyMap.size(); ++actionIndex) {
        out << actionIndex << " " << rewardCPF->actionHashKeyMap[actionIndex] << endl;
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

    out << endl << endl << "#####PRECONDITIONS#####" << endl;

    for(unsigned int index = 0; index < dynamicSACs.size(); ++index) {
        assert(dynamicSACs[index]->index == index);
        out << "## index" << endl;
        out << index << endl;
        out << "## formula" << endl;
        dynamicSACs[index]->formula->print(out);
        out << endl;
        out << "## hash index" << endl;
        out << dynamicSACs[index]->hashIndex << endl;
        out << "## caching type" << endl;
        out << dynamicSACs[index]->cachingType << endl;
        if(dynamicSACs[index]->cachingType == "VECTOR") {
            out << "## precomputed results" << endl;
            out << dynamicSACs[index]->precomputedResults.size() << endl;
            for(unsigned int res = 0; res < dynamicSACs[index]->precomputedResults.size(); ++res) {
                out << res << " " << dynamicSACs[index]->precomputedResults[res] << endl;
            }
        }
        out << "## kleene caching type" << endl;
        out << dynamicSACs[index]->kleeneCachingType << endl;
        if(dynamicSACs[index]->kleeneCachingType == "VECTOR") {
            out << "## kleene caching vec size" << endl;
            out << dynamicSACs[index]->kleeneCachingVectorSize << endl;
        }

        out << "## action hash keys" << endl;
        for(unsigned int actionIndex = 0; actionIndex < dynamicSACs[index]->actionHashKeyMap.size(); ++actionIndex) {
            out << actionIndex << " " << dynamicSACs[index]->actionHashKeyMap[actionIndex] << endl;
        }

        out << endl;
    }

    out << endl << endl << "#####ACTION STATES#####" << endl;
    for(unsigned int index = 0; index < actionStates.size(); ++index) {
        out << "## index" << endl;
        out << index << endl;
        out << "## state" << endl;
        for(unsigned int varIndex = 0; varIndex < actionStates[index].state.size(); ++varIndex) {
            out << actionStates[index][varIndex] << " ";
        }
        out << endl;
        out << "## relevant preconditions" << endl;
        out << actionStates[index].relevantSACs.size() << endl;
        for(unsigned int sacIndex = 0; sacIndex < actionStates[index].relevantSACs.size(); ++sacIndex) {
            out << actionStates[index].relevantSACs[sacIndex]->index << " ";
        }
        out << endl;
        out << endl;
    }

    out << endl << "#####HASH KEYS OF DETERMINISTIC STATE FLUENTS#####" << endl;
    for(unsigned int index = 0; index < firstProbabilisticVarIndex; ++index) {
        assert(CPFs[index]->head->index == index);
        out << "## index" << endl;
        out << index << endl;
        if(!stateHashKeys.empty()) {
            out << "## state hash key (for each value in the domain)" << endl;
            for(unsigned int valIndex = 0; valIndex < stateHashKeys[index].size(); ++valIndex) {
                out << stateHashKeys[index][valIndex];
                if(valIndex != stateHashKeys[index].size() - 1) {
                    out << " ";
                }
            }
        }
        out << endl;

        if(!kleeneStateHashKeyBases.empty()) {
            out << "## kleene state hash key base" << endl;
            out << kleeneStateHashKeyBases[index] << endl;
        }

        out << "## state fluent hash keys (first line is the number of keys)" << endl;
        out << indexToStateFluentHashKeyMap[index].size() << endl;
        for(unsigned int i = 0; i < indexToStateFluentHashKeyMap[index].size(); ++i) {
            out << indexToStateFluentHashKeyMap[index][i].first << " ";
            out << indexToStateFluentHashKeyMap[index][i].second << endl;
        }

        out << "## kleene state fluent hash keys (first line is the number of keys)" << endl;
        out << indexToKleeneStateFluentHashKeyMap[index].size() << endl;
        for(unsigned int i = 0; i < indexToKleeneStateFluentHashKeyMap[index].size(); ++i) {
            out << indexToKleeneStateFluentHashKeyMap[index][i].first << " ";
            out << indexToKleeneStateFluentHashKeyMap[index][i].second << endl;
        }
        out << endl;
    }

    out << endl << "#####HASH KEYS OF PROBABILISTIC STATE FLUENTS#####" << endl;
    for(unsigned int index = firstProbabilisticVarIndex; index < CPFs.size(); ++index) {
        assert(CPFs[index]->head->index == index);
        out << "## index" << endl;
        out << (index-firstProbabilisticVarIndex) << endl;
        if(!stateHashKeys.empty()) {
            out << "## state hash key (for each value in the domain)" << endl;
            for(unsigned int valIndex = 0; valIndex < stateHashKeys[index].size(); ++valIndex) {
                out << stateHashKeys[index][valIndex];
                if(valIndex != stateHashKeys[index].size() - 1) {
                    out << " ";
                }
            }
        }
        out << endl;

        if(!kleeneStateHashKeyBases.empty()) {
            out << "## kleene state hash key base" << endl;
            out << kleeneStateHashKeyBases[index] << endl;
        }

        out << "## state fluent hash keys (first line is the number of keys)" << endl;
        out << indexToStateFluentHashKeyMap[index].size() << endl;
        for(unsigned int i = 0; i < indexToStateFluentHashKeyMap[index].size(); ++i) {
            out << indexToStateFluentHashKeyMap[index][i].first << " ";
            out << indexToStateFluentHashKeyMap[index][i].second << endl;
        }

        out << "## kleene state fluent hash keys (first line is the number of keys)" << endl;
        out << indexToKleeneStateFluentHashKeyMap[index].size() << endl;
        for(unsigned int i = 0; i < indexToKleeneStateFluentHashKeyMap[index].size(); ++i) {
            out << indexToKleeneStateFluentHashKeyMap[index][i].first << " ";
            out << indexToKleeneStateFluentHashKeyMap[index][i].second << endl;
        }
        out << endl;
    }

    out << endl << endl << "#####TRAINING SET#####" << endl;
    out << trainingSet.size() << endl;
    for(set<State>::iterator it = trainingSet.begin(); it != trainingSet.end(); ++it) {
        for(unsigned int i = 0; i < it->state.size(); ++i) {
            out << it->state[i] << " ";
        }
        out << endl;
    }
}
