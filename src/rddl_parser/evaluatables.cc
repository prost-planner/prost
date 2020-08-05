#include "evaluatables.h"

#include "rddl.h"

#include "utils/system_utils.h"

#include <iostream>
#include <numeric>

using namespace std;

namespace prost {
namespace parser {
void Evaluatable::initialize() {
    isProb = false;
    hasArithmeticFunction = false;
    dependentStateFluents.clear();
    dependentActionFluents.clear();
    formula->collectInitialInfo(isProb, hasArithmeticFunction,
                                dependentStateFluents, dependentActionFluents);
}

void Evaluatable::simplify(Simplifications& replace) {
    formula = formula->simplify(replace);
    initialize();
}

void Evaluatable::initializeHashKeys(RDDLTask* task) {
    assert(hashIndex >= 0);
    long baseKey = initializeActionHashKeys(task);
    initializeStateFluentHashKeys(task, baseKey);
    initializeKleeneStateFluentHashKeys(task, baseKey);
}

// Assign a hash key to every action such that actions which have the same value
// for all action fluents affected by this evalutable are mapped to the same key
long Evaluatable::initializeActionHashKeys(RDDLTask* task) {
    int numActions = static_cast<int>(task->actionStates.size());
    long nextKey = 0;
    actionHashKeyMap = vector<long>(numActions, 0);
    for (int i = 0; i < numActions; ++i) {
        long existingKey = getActionHashKey(task->actionStates, i);
        if (existingKey == -1) {
            actionHashKeyMap[i] = nextKey;
            ++nextKey;
        } else {
            actionHashKeyMap[i] = existingKey;
        }
    }
    return nextKey;
}

// Determine if all relevant action fluents of one of the already assigned
// actions (i.e., an action up to action actionStates[actionIndex]) and
// actionState[actionIndex] are equal
long Evaluatable::getActionHashKey(
    vector<ActionState> const& actionStates, int actionIndex) {
    ActionState const& action = actionStates[actionIndex];
    for (int i = 0; i < actionIndex; ++i) {
        ActionState const& otherAction = actionStates[i];
        bool actionIsEquivalent = true;
        for (ActionFluent* af : dependentActionFluents) {
            if (action[af->index] != otherAction[af->index]) {
                actionIsEquivalent = false;
                break;
            }
        }
        if (actionIsEquivalent) {
            return actionHashKeyMap[i];
        }
    }
    return -1;
}

void Evaluatable::initializeStateFluentHashKeys(
    RDDLTask* task, long nextHashKeyBase) {
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
                !utils::MathUtils::multiplyWithOverflowCheck(
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

void Evaluatable::initializeKleeneStateFluentHashKeys(
    RDDLTask* task, long nextHashKeyBase) {
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
                !utils::MathUtils::multiplyWithOverflowCheck(
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

void ConditionalProbabilityFunction::setDomain(int maxVal) {
    domain.resize(maxVal + 1);
    iota(domain.begin(), domain.end(), 0);
    head->domainSize = maxVal + 1;
}
} // namespace parser
} // namespace prost
