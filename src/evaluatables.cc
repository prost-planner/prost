#include "evaluatables.h"

#include "planning_task.h"

#include "utils/system_utils.h"

using namespace std;

/*****************************************************************
                           Evaluatable
*****************************************************************/

void Evaluatable::initialize() {
    formula->collectInitialInfo(isProb, hasArithmeticFunction, dependentStateFluents, positiveActionDependencies, negativeActionDependencies);
}

void Evaluatable::initializeHashKeys(UnprocessedPlanningTask* task) {
    assert(hashIndex >= 0);

    long firstStateFluentHashKeyBase = initializeActionHashKeys(task->actionStates);
    initializeStateFluentHashKeys(task->CPFs, task->indexToStateFluentHashKeyMap, firstStateFluentHashKeyBase);
    initializeKleeneStateFluentHashKeys(task->CPFs, task->indexToKleeneStateFluentHashKeyMap, firstStateFluentHashKeyBase);
}

long Evaluatable::initializeActionHashKeys(vector<ActionState> const& actionStates) {
    long firstStateFluentHashKeyBase = 1;
    actionHashKeyMap = vector<long>(actionStates.size(), 0);

    for(unsigned int j =  0; j < actionStates.size(); ++j) {
        calculateActionHashKey(actionStates, actionStates[j], firstStateFluentHashKeyBase);
    }
    return firstStateFluentHashKeyBase;
}

void Evaluatable::calculateActionHashKey(vector<ActionState> const& actionStates, ActionState const& action, long& nextKey) {
    vector<ActionFluent*> depActs;
    for(unsigned int i = 0; i < action.scheduledActionFluents.size(); ++i) {
        if(dependsOnActionFluent(action.scheduledActionFluents[i])) {
            depActs.push_back(action.scheduledActionFluents[i]);
        }
    }

    if(!depActs.empty()) {
        if(depActs.size() == action.scheduledActionFluents.size()) {
            actionHashKeyMap[action.index] = nextKey;
            ++nextKey;
        } else {
            long key = getActionHashKey(actionStates, depActs);
            if(key != -1) {
                actionHashKeyMap[action.index] = key;
            } else {
                actionHashKeyMap[action.index] = nextKey;
                ++nextKey;
            }
        }
    }
}
    
long Evaluatable::getActionHashKey(vector<ActionState> const& actionStates, vector<ActionFluent*>& scheduledActions) {
    for(unsigned int i = 0; i < actionStates.size(); ++i) {
        if(actionStates[i].scheduledActionFluents == scheduledActions) {
            return actionHashKeyMap[i];
        }
    }
    return -1;
}

void Evaluatable::initializeStateFluentHashKeys(vector<ConditionalProbabilityFunction*> const& CPFs,
                                                vector<vector<pair<int,long> > >& indexToStateFluentHashKeyMap,
                                                long const& firstStateFluentHashKeyBase) {
    long nextHashKeyBase = firstStateFluentHashKeyBase;

    // We use this to store the state fluent update rules temporary as it is
    // possible that this evaluatable cannot use caching. This evaluatable
    // depends on the variables tmpStateFluentDependencies[i].first, and its
    // StateFluentHashKey is thereby increased by that variable's value
    // multiplied with tmpStateFluentDependencies[i].second
    vector<pair<int, long> > tmpStateFluentDependencies;

    // We iterate over the CPFs instead of directly over the
    // dependentStateFluents vector as we have to access the CPF objects
    for(unsigned int index = 0; index < CPFs.size(); ++index) {
        if(dependentStateFluents.find(CPFs[index]->getHead()) != dependentStateFluents.end()) {
            tmpStateFluentDependencies.push_back(make_pair(index, nextHashKeyBase));

            if(!CPFs[index]->hasFiniteDomain() || !MathUtils::multiplyWithOverflowCheck(nextHashKeyBase, CPFs[index]->getDomainSize())) {
                cachingType = NONE;
                return;
            }
        }
    }

    for(unsigned int index = 0; index < tmpStateFluentDependencies.size(); ++index) {
        indexToStateFluentHashKeyMap[tmpStateFluentDependencies[index].first].push_back(make_pair(hashIndex, tmpStateFluentDependencies[index].second));
    }

    // TODO: Make sure this number makes sense
    if(nextHashKeyBase > 50000) {
        cachingType = MAP;
    } else {
        cachingType = VECTOR;
        evaluationCacheVector = vector<double>(nextHashKeyBase, -numeric_limits<double>::max());
        if(isProbabilistic()) {
            pdEvaluationCacheVector = vector<DiscretePD>(nextHashKeyBase, DiscretePD());
        }
    }
}

void Evaluatable::initializeKleeneStateFluentHashKeys(vector<ConditionalProbabilityFunction*> const& CPFs,
                                                      vector<vector<pair<int,long> > >& indexToKleeneStateFluentHashKeyMap,
                                                      long const& firstStateFluentHashKeyBase) {
    long nextHashKeyBase = firstStateFluentHashKeyBase;

    // We use this to store the state fluent update rules temporary as it is
    // possible that this evaluatable cannot use caching. This evaluatable
    // depends on the variables tmpStateFluentDependencies[i].first, and its
    // KleeneStateFluentHashKey is thereby increased by that variable's value
    // multiplied with tmpStateFluentDependencies[i].second
    vector<pair<int, long> > tmpStateFluentDependencies;

    // We iterate over the CPFs instead of directly over the
    // dependentStateFluents vector as we have to access the CPF objects
    for(unsigned int index = 0; index < CPFs.size(); ++index) {
        if(dependentStateFluents.find(CPFs[index]->getHead()) != dependentStateFluents.end()) {
            tmpStateFluentDependencies.push_back(make_pair(index, nextHashKeyBase));

            if((CPFs[index]->getKleeneDomainSize() < 0) || !MathUtils::multiplyWithOverflowCheck(nextHashKeyBase, CPFs[index]->getKleeneDomainSize())) {
                kleeneCachingType = NONE;
                return;
            }
        }
    }

    for(unsigned int index = 0; index < tmpStateFluentDependencies.size(); ++index) {
        indexToKleeneStateFluentHashKeyMap[tmpStateFluentDependencies[index].first].push_back(make_pair(hashIndex, tmpStateFluentDependencies[index].second));
    }

    // TODO: Make sure this number makes sense
    if(nextHashKeyBase > 50000) {
        kleeneCachingType = MAP;
    } else {
        kleeneCachingType = VECTOR;
        kleeneEvaluationCacheVector = vector<set<double> >(nextHashKeyBase);
    }
}

void Evaluatable::disableCaching() {
    //We only disable caching if it is done in maps as the 
    //space for vectors is already reserved and thus not growing.
    if(cachingType == MAP) {
        cachingType = DISABLED_MAP;
    }

    if(kleeneCachingType == MAP) {
        kleeneCachingType = DISABLED_MAP;
    }
}

