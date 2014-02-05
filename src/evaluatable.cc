#include "evaluatable.h"

#include "planning_task.h"

#include "utils/system_utils.h"

using namespace std;

/*****************************************************************
                           Initialization
*****************************************************************/

void Evaluatable::initialize() {
    formula->collectInitialInfo(isProb, hasArithmeticFunction, dependentStateFluents, positiveActionDependencies, negativeActionDependencies);
}

void Evaluatable::initializeHashKeys(int _hashIndex, vector<ActionState> const& actionStates,
                                     vector<ConditionalProbabilityFunction*> const& CPFs,
                                     vector<vector<pair<int,long> > >& indexToStateFluentHashKeyMap,
                                     vector<vector<pair<int,long> > >& indexToKleeneStateFluentHashKeyMap) {
    hashIndex = _hashIndex;

    initializeActionHashKeys(actionStates);
    initializeStateFluentHashKeys(CPFs, indexToStateFluentHashKeyMap);
    initializeKleeneStateFluentHashKeys(CPFs, indexToKleeneStateFluentHashKeyMap);
}

void Evaluatable::initializeActionHashKeys(vector<ActionState> const& actionStates) {
    firstStateFluentHashKeyBase = 1;
    actionHashKeyMap = vector<long>(actionStates.size(), 0);

    for(unsigned int j =  0; j < actionStates.size(); ++j) {
        calculateActionHashKey(actionStates, actionStates[j], firstStateFluentHashKeyBase);
    }
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
                                                vector<vector<pair<int,long> > >& indexToStateFluentHashKeyMap) {
    long nextHashKeyBase = firstStateFluentHashKeyBase;

    // We use this to store the state fluent update rules temporary as
    // it is possible that this evaluatable cannot use caching.
    vector<pair<int, long> > tmpStateFluentDependencies;

    for(unsigned int i = 0; i < CPFs.size(); ++i) {
        set<StateFluent*>::iterator it = dependentStateFluents.find(CPFs[i]->getHead());
        if(it != dependentStateFluents.end()) {
            tmpStateFluentDependencies.push_back(make_pair((*it)->index, nextHashKeyBase));

            if((CPFs[i]->getDomainSize() == 0) || !MathUtils::multiplyWithOverflowCheck(nextHashKeyBase, CPFs[i]->getDomainSize())) {
                cachingType = NONE;
                return;
            }
        }
    }

    for(unsigned int i = 0; i < tmpStateFluentDependencies.size(); ++i) {
        indexToStateFluentHashKeyMap[tmpStateFluentDependencies[i].first].push_back(make_pair(hashIndex, tmpStateFluentDependencies[i].second));
    }

    if(nextHashKeyBase > 50000) {
        cachingType = MAP;
    } else {
        cachingType = VECTOR;
        evaluationCacheVector = vector<DiscretePD>(nextHashKeyBase, DiscretePD());
    }
}

void Evaluatable::initializeKleeneStateFluentHashKeys(vector<ConditionalProbabilityFunction*> const& CPFs,
                                                      vector<vector<pair<int,long> > >& indexToKleeneStateFluentHashKeyMap) {
    //REPAIR
    kleeneCachingType = NONE;
    return;

    long nextHashKeyBase = firstStateFluentHashKeyBase;

    // We use this to store the state fluent update rules temporary as
    // it is possible that this evaluatable cannot use caching.
    vector<pair<int, long> > tmpStateFluentDependencies;

    for(unsigned int i = 0; i < CPFs.size(); ++i) {
        set<StateFluent*>::iterator it = dependentStateFluents.find(CPFs[i]->getHead());
        if(it != dependentStateFluents.end()) {
            tmpStateFluentDependencies.push_back(make_pair((*it)->index, nextHashKeyBase));

            if(!MathUtils::multiplyWithOverflowCheck(nextHashKeyBase, 3)) {
                kleeneCachingType = NONE;
                return;
            }
        }
    }

    for(unsigned int i = 0; i < tmpStateFluentDependencies.size(); ++i) {
        indexToKleeneStateFluentHashKeyMap[tmpStateFluentDependencies[i].first].push_back(make_pair(hashIndex, tmpStateFluentDependencies[i].second));
    }

    if(nextHashKeyBase > 50000) {
        kleeneCachingType = MAP;
    } else {
        kleeneCachingType = VECTOR;
        kleeneEvaluationCacheVector = vector<double>(nextHashKeyBase, -numeric_limits<double>::max());
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
