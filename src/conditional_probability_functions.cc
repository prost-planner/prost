#include "conditional_probability_functions.h"

#include "prost_planner.h"
#include "actions.h"
#include "rddl_parser.h"

#include "utils/string_utils.h"

#include <iostream>
#include <cassert>

using namespace std;

/*****************************************************************
                 ConditionalProbabilityFunction
*****************************************************************/

void ConditionalProbabilityFunction::disableCaching() {
    //We only disable caching if it is done in maps as the 
    //space for vectors is already reserved and thus not growing.
    if(!cacheInVector && cachingEnabled) {
        cachingEnabled = false;
    }

    if(!kleeneCacheInVector && kleeneCachingEnabled) {
        kleeneCachingEnabled = false;
    }
}

bool ConditionalProbabilityFunction::simplify(UnprocessedPlanningTask* _task, map<StateFluent*, NumericConstant*>& replacements) {
    body = body->simplify(_task, replacements);
    NumericConstant* nc = dynamic_cast<NumericConstant*>(body);
    if(nc) {
        assert(replacements.find(head) == replacements.end());
        replacements[head] = nc;
        return true;
    }
    return false;
}

void ConditionalProbabilityFunction::initialize() {
    assert(!initialized);

    std::vector<ActionFluent*> positiveActionDependencies;
    std::vector<ActionFluent*> negativeActionDependencies;
    body->collectInitialInfo(isProb, dependentStateFluents, positiveActionDependencies, negativeActionDependencies);

    if(positiveActionDependencies.empty()) {
        doesNotDependPositivelyOnActs = true; 
    } else {
        doesNotDependPositivelyOnActs = false;
    }

    for(unsigned int i = 0; i < positiveActionDependencies.size(); ++i) {
        bool alreadyIn = false;
        for(unsigned int j = 0; j < dependentActionFluents.size(); ++j) {
            if(positiveActionDependencies[i] == dependentActionFluents[j]) {
                alreadyIn = true;
                break;
            }
        }
        if(!alreadyIn) {
            dependentActionFluents.push_back(positiveActionDependencies[i]);
        }
    }

    for(unsigned int i = 0; i < negativeActionDependencies.size(); ++i) {
        bool alreadyIn = false;
        for(unsigned int j = 0; j < dependentActionFluents.size(); ++j) {
            if(negativeActionDependencies[i] == dependentActionFluents[j]) {
                alreadyIn = true;
                break;
            }
        }
        if(!alreadyIn) {
            dependentActionFluents.push_back(negativeActionDependencies[i]);
        }
    }

    if(dependentActionFluents.empty()) {
        isActionIndep = true;
    } else {
        isActionIndep = false;
    }

    initialized = true;
}

void ConditionalProbabilityFunction::initializeStateFluentHashKeys() {
    if(stateFluentHashKeysCalculated) {
        return;
    }
    stateFluentHashKeysCalculated = true;

    long nextKey = 1;
    actionHashKeyMap = vector<long>(task->getNumberOfActions(), 0);

    for(unsigned int j =  0; j < task->getNumberOfActions(); ++j) {
        calculateActionHashKey(task->actionState(j), nextKey);
    }
    --nextKey;
    int nextExponent = 0;

    while(MathUtils::twoToThePowerOf(nextExponent) <= nextKey) {
        ++nextExponent;
    }

    if(nextExponent + dependentStateFluents.size() > MathUtils::twoToThePowerOfMap.size()) {
        cacheInVector = false;
        cachingEnabled = false;
        return;
    }

    for(unsigned int i = 0; i < task->getStateSize(); ++i) {
        for(unsigned int j = 0; j < dependentStateFluents.size(); ++j) {
            if(task->CPFs[i]->head == dependentStateFluents[j]) {
                task->indexToStateFluentHashKeyMap[dependentStateFluents[j]->index].push_back(make_pair(index,MathUtils::twoToThePowerOf(nextExponent)));
                ++nextExponent;
                break;
            }
        }
    }

    if(nextExponent >= 15) {
        cacheInVector = false;
    } else {
        cacheInVector = true;
        evaluationCacheVector = vector<double>(nextKey+MathUtils::twoToThePowerOf(nextExponent), -numeric_limits<double>::max());
    }
}

void ConditionalProbabilityFunction::initializeKleeneStateFluentHashKeys() {
    if(kleeneStateFluentHashKeysCalculated) {
        return;
    }
    kleeneStateFluentHashKeysCalculated = true;

    long nextKey = 1;
    actionHashKeyMap = vector<long>(task->getNumberOfActions(), 0);

    for(unsigned int j =  0; j < task->getNumberOfActions(); ++j) {
        calculateActionHashKey(task->actionState(j), nextKey);
    }
    --nextKey;
    int nextExponent = 0;

    while(MathUtils::threeToThePowerOf(nextExponent) <= nextKey) {
        ++nextExponent;
    }

    if(nextExponent + dependentStateFluents.size() > MathUtils::threeToThePowerOfMap.size()) {
        kleeneCacheInVector = false;
        kleeneCachingEnabled = false;
        return;
    }

    for(unsigned int i = 0; i < task->getStateSize(); ++i) {
        for(unsigned int j = 0; j < dependentStateFluents.size(); ++j) {
            if(task->CPFs[i]->head == dependentStateFluents[j]) {
                task->indexToKleeneStateFluentHashKeyMap[dependentStateFluents[j]->index].push_back(make_pair(index,MathUtils::threeToThePowerOf(nextExponent)));
                ++nextExponent;
                break;
            }
        }
    }

    if(nextExponent >= 10) {
        kleeneCacheInVector = false;
    } else {
        kleeneCacheInVector = true;
        //++nextExponent;
        kleeneEvaluationCacheVector = vector<double>(nextKey+(2*MathUtils::threeToThePowerOf(nextExponent)), -numeric_limits<double>::max());
    }
}

void ConditionalProbabilityFunction::calculateActionHashKey(ActionState const& action, long& nextKey) {
    vector<ActionFluent*> depActs;
    for(unsigned int i = 0; i < action.scheduledActionFluents.size(); ++i) {
        for(unsigned int j = 0; j < dependentActionFluents.size(); ++j) {
            if(action.scheduledActionFluents[i] == dependentActionFluents[j]) {
                depActs.push_back(dependentActionFluents[j]);
            }
        }
    }

    if(!depActs.empty()) {
        if(depActs.size() == action.scheduledActionFluents.size()) {
            actionHashKeyMap[action.index] = nextKey;
            ++nextKey;
        } else {
            long key = getActionHashKey(depActs);
            if(key != -1) {
                actionHashKeyMap[action.index] = key;
            } else {
                actionHashKeyMap[action.index] = nextKey;
                ++nextKey;
            }
        }
    }
}

long ConditionalProbabilityFunction::getActionHashKey(vector<ActionFluent*>& scheduledActions) {
    for(unsigned int i = 0; i < task->getNumberOfActions(); ++i) {
        if(task->actionState(i).scheduledActionFluents == scheduledActions) {
            return actionHashKeyMap[i];
        }
    }
    return -1;
}

void ConditionalProbabilityFunction::calculateDomain(vector<ActionState>& actions) {
    assert(!domainCalculated);
    assert(initialized);

    //TODO: Remove the probabilistic check for object fluent support!
    if(isProbabilistic() || head == StateFluent::rewardInstance()) {
        set<double> allVals;
        for(unsigned int i = 0; i < actions.size(); ++i) {
            set<double> tmp;
            body->calculateDomain(actions[i], tmp);
            for(set<double>::iterator it = tmp.begin(); it != tmp.end(); ++it) {
                if(allVals.find(*it) == allVals.end()) {
                    allVals.insert(*it);
                }
            }
        }

        for(set<double>::iterator it = allVals.begin(); it != allVals.end(); ++it) {
            probDomainMap[*it] = probDomainSize;
            ++probDomainSize;
        }
        minVal = *(allVals.begin());
        maxVal = *(allVals.rbegin());
    } else {
        probDomainSize = 2;
        probDomainMap[0.0] = 0;
        probDomainMap[1.0] = 1;
        minVal = 0;
        maxVal = 0;
    }

    domainCalculated = true;
}

ConditionalProbabilityFunction* ConditionalProbabilityFunction::determinizeMostLikely(NumericConstant* randomNumberReplacement, 
                                                                                      PlanningTask* detPlanningTask, 
                                                                                      UnprocessedPlanningTask* _task) {
    //If this is not probabilistic, we return this to reuse the caches
    //as often as possible. This is nevertheless a potential error
    //source that should be kept in mind!
    if(!isProb) {
        return this;
    }

    LogicalExpression* detBody = body->determinizeMostLikely(randomNumberReplacement);
    ConditionalProbabilityFunction* res = new ConditionalProbabilityFunction(*this);
    res->task = detPlanningTask;
    res->body = detBody;

    //we know these because detBody must be deterministic, and
    //therefore this has a domain of 2 with values 0 and 1. 
    res->isProb = false;
    res->probDomainSize = 2;
    res->probDomainMap.clear();
    res->probDomainMap[0.0] = 0;
    res->probDomainMap[1.0] = 1;

    //the probHashKeyBase is equal to the hashKeybase as we only have
    //2 possible values
    res->probHashKeyBase = res->hashKeyBase;

    //we also reset the caches
    res->evaluationCacheVector = vector<double>(evaluationCacheVector.size(),-numeric_limits<double>::max());
    res->kleeneEvaluationCacheVector = vector<double>(kleeneEvaluationCacheVector.size(),-numeric_limits<double>::max());

    //some assertions that must be true for this and should have beend
    //copied correctly
    assert(res->stateFluentHashKeysCalculated);
    assert(res->kleeneStateFluentHashKeysCalculated);

    //We run initialization again, as things might have changed
    //compared to the probabilistic task. Therefore, we need to reset
    //some member variables to their initial value
    res->initialized = false;
    res->dependentStateFluents.clear();
    res->dependentActionFluents.clear();
    res->initialize();

    //The same is true for simplification. Rather than calling
    //simplify(), though, we call body->simplify. This is because the
    //function also checks if this CPF can be omitted entirely, which
    //is never true in a determinization
    map<StateFluent*,NumericConstant*> replacements;
    res->body = res->body->simplify(_task, replacements);

    return res;
}

void ConditionalProbabilityFunction::print(ostream& out) {
    head->print(out);
    out << " (Index: " << head->index << ")";
    if(initialized) {
        if(!isProb) {
            out << " (deterministic,";
        } else {
            out << " (probabilistic,";
        }
        if(isActionIndep) {
            out << " action independent,";
        } else {
            out << " action dependent,";
        }
        if(cacheInVector) {
            out << " caching in vectors of size " << evaluationCacheVector.size() << ", ";
        } else {
            if(cachingEnabled) {
                out << " caching in maps, ";
            } else {
                out << " no caching, ";
            }
        }
        if(kleeneCacheInVector) {
            out << " kleene caching in vectors of size " << kleeneEvaluationCacheVector.size() << ")" << endl;
        } else {
            if(kleeneCachingEnabled) {
                out << " kleene caching in maps)" << endl;
            } else {
                out << " no kleene caching)" << endl;
            }
        }
    }
    body->print(out);

    if(domainCalculated) {
        out << endl << endl << "Possible values (" << probDomainSize << "): " << endl;

        for(map<double,int>::iterator it = probDomainMap.begin(); it != probDomainMap.end(); ++it) {
            out << it->first << " (" << it->second << ") ";
        }
        out << endl;

        if(head != StateFluent::rewardInstance()) {
            out << "ProbHashKeyBase is " << probHashKeyBase << " and hashKeybase is " << hashKeyBase << endl;
        }
    }

    if(stateFluentHashKeysCalculated) {
        out << endl << endl << "Hash Keys of dependent Fluents: " << endl;
        for(unsigned int i = 0; i < actionHashKeyMap.size(); ++i) {
            if(actionHashKeyMap[i] != 0) {
                out << "Action Fluent ";
                task->printAction(out, i);
                out << " : " << actionHashKeyMap[i] << endl;
            }
        }
    } else {
        out << endl << endl;
    }
}

/*****************************************************************
           ConditionalProbabilityFunctionDefinition
*****************************************************************/

void ConditionalProbabilityFunctionDefinition::parse(string& desc, UnprocessedPlanningTask* task, RDDLParser* parser) {
    size_t cutPos = desc.find("=");
    assert(cutPos != string::npos);

    string nameAndParams = desc.substr(0,cutPos);
    StringUtils::trim(nameAndParams);
    string rest = desc.substr(cutPos+1);
    StringUtils::trim(rest);

    string name;
    vector<string> params;
    StringUtils::trim(nameAndParams);

    if(nameAndParams.compare("reward") != 0) {
        StringUtils::removeFirstAndLastCharacter(nameAndParams);
    }

    vector<string> nameAndParamsVec;
    StringUtils::split(nameAndParams,nameAndParamsVec," ");

    assert(nameAndParamsVec.size() > 0);
    name = nameAndParamsVec[0];
    StringUtils::trim(name);

    if(name[name.length()-1] == '\'') {
        name = name.substr(0,name.length()-1);
    }

    VariableDefinition* headParent = task->getVariableDefinition(name);

    vector<string> headParams;
    for(unsigned int i = 1; i < nameAndParamsVec.size(); ++i) {
        headParams.push_back(nameAndParamsVec[i]);
    }
    UninstantiatedVariable* head = new UninstantiatedVariable(headParent, headParams);

    LogicalExpression* body = parser->parseRDDLFormula(rest,task);

    task->addCPFDefinition(new ConditionalProbabilityFunctionDefinition(head,body));
}

void ConditionalProbabilityFunctionDefinition::replaceQuantifier(UnprocessedPlanningTask* task, Instantiator* instantiator) {
    map<string, string> replacements;
    body = body->replaceQuantifier(task, replacements, instantiator);
}

ConditionalProbabilityFunction* ConditionalProbabilityFunctionDefinition::instantiate(ProstPlanner* planner,
                                                                                      UnprocessedPlanningTask* task, 
                                                                                      PlanningTask* preprocessedPlanningTask, 
                                                                                      AtomicLogicalExpression* variable) {
    assert(head->params.size() == variable->params.size());

    map<string, Object*> replacements;
    for(unsigned int i = 0; i < head->params.size(); ++i) {
        assert(replacements.find(head->params[i]) == replacements.end());
        replacements[head->params[i]] = variable->params[i];
    }
    LogicalExpression* newBody = body->instantiate(task,replacements);
    StateFluent* tmp = dynamic_cast<StateFluent*>(variable);
    assert(tmp != NULL);
    return new ConditionalProbabilityFunction(planner, preprocessedPlanningTask, tmp, newBody);
}

void ConditionalProbabilityFunctionDefinition::print(ostream& out) {
    head->print(out);
    out << " = ";
    body->print(out);
    out << endl;
}
