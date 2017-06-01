#include "evaluatables.h"

#include "rddl.h"

#include "utils/system_utils.h"

#include <iostream>

using namespace std;

/*****************************************************************
                           Evaluatable
*****************************************************************/

void Evaluatable::initialize() {
    isProb = false;
    hasArithmeticFunction = false;
    dependentStateFluents.clear();
    dependentActionFluents.clear();

    formula->collectInitialInfo(isProb, hasArithmeticFunction,
                                dependentStateFluents, dependentActionFluents);
}

void ActionPrecondition::initialize() {
    Evaluatable::initialize();

    positiveActionDependencies.clear();
    negativeActionDependencies.clear();

    formula->classifyActionFluents(positiveActionDependencies,
                                   negativeActionDependencies);
}

void RewardFunction::initialize() {
    Evaluatable::initialize();

    positiveActionDependencies.clear();
    negativeActionDependencies.clear();

    formula->classifyActionFluents(positiveActionDependencies,
                                   negativeActionDependencies);

    // cout << "Action fluents: " << endl;
    // for(set<ActionFluent*>::iterator it = dependentActionFluents.begin(); it
    // != dependentActionFluents.end(); ++it) {
    //     cout << "  " << (*it)->fullName << endl;
    // }
    // cout << endl;

    // cout << "Positive action fluents: " << endl;
    // for(set<ActionFluent*>::iterator it = positiveActionDependencies.begin();
    // it != positiveActionDependencies.end(); ++it) {
    //     cout << "  " << (*it)->fullName << endl;
    // }
    // cout << endl;

    // cout << "Negative action fluents: " << endl;
    // for(set<ActionFluent*>::iterator it = negativeActionDependencies.begin();
    // it != negativeActionDependencies.end(); ++it) {
    //     cout << "  " << (*it)->fullName << endl;
    // }
    // cout << endl;
}

void Evaluatable::simplify(map<ParametrizedVariable*, double>& replace) {
    formula = formula->simplify(replace);

    initialize();
}

void Evaluatable::initializeHashKeys(RDDLTask* task) {
    assert(hashIndex >= 0);

    long baseKey = initializeActionHashKeys(task->actionStates);
    initializeStateFluentHashKeys(task, baseKey);
    initializeKleeneStateFluentHashKeys(task, baseKey);
}

long Evaluatable::initializeActionHashKeys(
    vector<ActionState> const& actionStates) {
    long baseKey = 1;
    actionHashKeyMap = vector<long>(actionStates.size(), 0);
    bool dependsOnAllActions = true;
    for (unsigned int actionIndex = 0; actionIndex < actionStates.size();
         ++actionIndex) {
        dependsOnAllActions &= calculateActionHashKey(
            actionStates, actionStates[actionIndex], baseKey);
    }
    if (dependsOnAllActions) {
        // If an action hash key is assigned to all actions, the key '0' is
        // unused -> decrease all keys by 1 (otherwise, there are 'gaps' in the
        // list of precomputed results). Note that this is only possible if
        // 'noop' is not prohibited by an action precondition.
        for (unsigned int i = 0; i < actionHashKeyMap.size(); ++i) {
            assert(actionHashKeyMap[i] > 0);
            --actionHashKeyMap[i];
        }
        --baseKey;
    }
    return baseKey;
}

bool Evaluatable::calculateActionHashKey(
    vector<ActionState> const& actionStates, ActionState const& action,
    long& nextKey) {
    vector<ActionFluent*> depActs;
    for (unsigned int i = 0; i < action.scheduledActionFluents.size(); ++i) {
        if (dependentActionFluents.find(action.scheduledActionFluents[i]) !=
            dependentActionFluents.end()) {
            depActs.push_back(action.scheduledActionFluents[i]);
        }
    }

    if (!depActs.empty()) {
        if (depActs.size() == action.scheduledActionFluents.size()) {
            actionHashKeyMap[action.index] = nextKey;
            ++nextKey;
            return true;
        } else {
            long key = getActionHashKey(actionStates, depActs);
            if (key != -1) {
                actionHashKeyMap[action.index] = key;
                return true;
            } else {
                actionHashKeyMap[action.index] = nextKey;
                ++nextKey;
                return true;
            }
        }
    }
    return false;
}

long Evaluatable::getActionHashKey(vector<ActionState> const& actionStates,
                                   vector<ActionFluent*>& scheduledActions) {
    for (unsigned int i = 0; i < actionStates.size(); ++i) {
        if (actionStates[i].scheduledActionFluents == scheduledActions) {
            return actionHashKeyMap[i];
        }
    }
    return -1;
}

void Evaluatable::initializeStateFluentHashKeys(RDDLTask* task,
                                                long const& baseKey) {
    long nextHashKeyBase = baseKey;

    // We use this to store the state fluent update rules temporary as it is
    // possible that this evaluatable cannot use caching. This evaluatable
    // depends on the variables tmpHashMap[i].first, and its StateFluentHashKey
    // is thereby increased by that variable's value multiplied with
    // tmpHashMap[i].second
    vector<pair<int, long>> tmpHashMap;

    // We iterate over the CPFs instead of directly over the
    // dependentStateFluents vector as we have to access the CPF objects
    for (unsigned int index = 0; index < task->CPFs.size(); ++index) {
        if (dependentStateFluents.find(task->CPFs[index]->head) !=
            dependentStateFluents.end()) {
            tmpHashMap.push_back(make_pair(index, nextHashKeyBase));

            if (!task->CPFs[index]->hasFiniteDomain() ||
                !MathUtils::multiplyWithOverflowCheck(
                    nextHashKeyBase, task->CPFs[index]->getDomainSize())) {
                cachingType = "NONE";
                return;
            }
        }
    }

    vector<vector<pair<int, long>>>& hashMap =
        task->indexToStateFluentHashKeyMap;

    for (unsigned int index = 0; index < tmpHashMap.size(); ++index) {
        hashMap[tmpHashMap[index].first].push_back(
            make_pair(hashIndex, tmpHashMap[index].second));
        stateFluentHashKeyBases.push_back(tmpHashMap[index]);
    }

    // TODO: Make sure this number makes sense
    if (nextHashKeyBase > 1000000) {
        cachingType = "MAP";
    } else {
        cachingType = "VECTOR";
        precomputedResults.resize(nextHashKeyBase,
                                  -numeric_limits<double>::max());
        if (isProbabilistic()) {
            precomputedPDResults.resize(nextHashKeyBase);
        }
    }
}

void Evaluatable::initializeKleeneStateFluentHashKeys(RDDLTask* task,
                                                      long const& baseKey) {
    long nextHashKeyBase = baseKey;

    // We use this to store the state fluent update rules temporary as it is
    // possible that this evaluatable cannot use caching. This evaluatable
    // depends on the variables tmpHashMap[i].first, and its
    // KleeneStateFluentHashKey is thereby increased by that variable's value
    // multiplied with tmpHashMap[i].second
    vector<pair<int, long>> tmpHashMap;

    // We iterate over the CPFs instead of directly over the
    // dependentStateFluents vector as we have to access the CPF objects
    for (unsigned int index = 0; index < task->CPFs.size(); ++index) {
        if (dependentStateFluents.find(task->CPFs[index]->head) !=
            dependentStateFluents.end()) {
            tmpHashMap.push_back(make_pair(index, nextHashKeyBase));

            if ((task->CPFs[index]->kleeneDomainSize == 0) ||
                !MathUtils::multiplyWithOverflowCheck(
                    nextHashKeyBase, task->CPFs[index]->kleeneDomainSize)) {
                kleeneCachingType = "NONE";
                return;
            }
        }
    }

    vector<vector<pair<int, long>>>& hashMap =
        task->indexToKleeneStateFluentHashKeyMap;

    for (unsigned int index = 0; index < tmpHashMap.size(); ++index) {
        hashMap[tmpHashMap[index].first].push_back(
            make_pair(hashIndex, tmpHashMap[index].second));
    }

    // TODO: Make sure this number makes sense
    if (nextHashKeyBase > 200000) {
        kleeneCachingType = "MAP";
    } else {
        kleeneCachingType = "VECTOR";
        kleeneCachingVectorSize = nextHashKeyBase;
    }
}
