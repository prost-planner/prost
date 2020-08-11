#include "hash_keys.h"
#include "../evaluatables.h"
#include "../rddl.h"

#include "../utils/math_utils.h"
#include "../utils/timer.h"

#include <algorithm>
#include <iostream>

using namespace std;

namespace prost::parser::hashing {
void HashKeyGenerator::generateHashKeys(bool output) {
    utils::Timer t;
    if (output) {
        cout << "    Preparing state hash keys..." << endl;
    }
    prepareHashKeysForStates();
    if (output) {
        cout << "    ...finished (" << t() << ")" << endl;
    }
    t.reset();

    if (output) {
        cout << "    Preparing Kleene state hash keys..." << endl;
    }
    prepareHashKeysForKleeneStates();
    if (output) {
        cout << "    ...finished (" << t() << ")" << endl;
    }
    t.reset();

    if (output) {
        cout << "    Preparing hash keys of evaluatables..." << endl;
    }
    prepareHashKeysForEvaluatables();
    if (output) {
        cout << "    ...finished (" << t() << ")" << endl;
    }
}

void HashKeyGenerator::prepareHashKeysForStates() {
    long hashKeyBase = 1;
    vector<vector<long>> stateHashKeys;

    for (ConditionalProbabilityFunction const* cpf : task->CPFs) {
        int numVals = cpf->getDomainSize();
        vector<long> stateHashKeysOfCPF(numVals);
        for (size_t val = 0; val < numVals; ++val) {
            stateHashKeysOfCPF[val] = val * hashKeyBase;
        }

        if (!utils::MathUtils::multiplyWithOverflowCheck(
            hashKeyBase, numVals)) {
            return;
        }
        stateHashKeys.push_back(move(stateHashKeysOfCPF));
    }
    task->stateHashKeys = move(stateHashKeys);
}

void HashKeyGenerator::prepareHashKeysForKleeneStates() {
    bool kleeneStateHashingPossible = true;
    int maxNumVals = (sizeof(long) * 8) - 2;
    // Compute the domain size of a state fluent w.r.t to KleeneStates as the
    // size of the power set of the domain of the state fluent minus 1
    for (ConditionalProbabilityFunction* cpf : task->CPFs) {
        if (cpf->getDomainSize() > maxNumVals) {
            cpf->kleeneDomainSize = 0;
            kleeneStateHashingPossible = false;
        } else {
            cpf->kleeneDomainSize = (1 << cpf->getDomainSize()) - 1;
        }
    }
    if (!kleeneStateHashingPossible) {
        return;
    }

    vector<long> hashKeyBases;
    long hashKeyBase = 1;
    for (ConditionalProbabilityFunction const* cpf : task->CPFs) {
        hashKeyBases.push_back(hashKeyBase);
        if (!utils::MathUtils::multiplyWithOverflowCheck(
            hashKeyBase, cpf->kleeneDomainSize)) {
            return;
        }
    }
    task->kleeneStateHashKeyBases = move(hashKeyBases);
}

void HashKeyGenerator::prepareHashKeysForEvaluatables() {
    task->indexToStateFluentHashKeyMap.resize(task->CPFs.size());
    task->indexToKleeneStateFluentHashKeyMap.resize(task->CPFs.size());

    int hashIndex = 0;
    for (ConditionalProbabilityFunction* cpf : task->CPFs) {
        prepareHashKeysForEvaluatable(cpf, hashIndex);
        ++hashIndex;
    }
    prepareHashKeysForEvaluatable(task->rewardCPF, hashIndex);
    for (ActionPrecondition* precond : task->preconds) {
        ++hashIndex;
        prepareHashKeysForEvaluatable(precond, hashIndex);
    }
}

void HashKeyGenerator::prepareHashKeysForEvaluatable(Evaluatable* eval,
                                                     int hashIndex) {
    eval->hashIndex = hashIndex;
    long baseKey = determineActionHashKeys(eval);
    prepareStateFluentHashKeys(eval, baseKey);
    prepareKleeneStateFluentHashKeys(eval, baseKey);
}

long HashKeyGenerator::determineActionHashKeys(Evaluatable* eval) {
    int numActions = static_cast<int>(task->actionStates.size());
    long nextKey = 0;
    eval->actionHashKeyMap = vector<long>(numActions, 0);
    for (int i = 0; i < numActions; ++i) {
        long existingKey = getActionHashKey(eval, i);
        if (existingKey == -1) {
            eval->actionHashKeyMap[i] = nextKey;
            ++nextKey;
        } else {
            eval->actionHashKeyMap[i] = existingKey;
        }
    }
    return nextKey;
}

long HashKeyGenerator::getActionHashKey(Evaluatable* eval, int actionIndex) {
    ActionState const& action = task->actionStates[actionIndex];
    for (int i = 0; i < actionIndex; ++i) {
        ActionState const& otherAction = task->actionStates[i];
        if (all_of(eval->dependentActionFluents.begin(),
                   eval->dependentActionFluents.end(),
                   [&](ActionFluent* af) {
                       return action[af->index] == otherAction[af->index];
                   })) {
            return eval->actionHashKeyMap[i];
        }
    }
    return -1;
}

void HashKeyGenerator::prepareStateFluentHashKeys(Evaluatable* eval,
                                                  long hashKeyBase) {
    // tmpHashMap[i].first is the index of a state variable that affects eval,
    // and the hash key base of that state variable is tmpHashMap[i].second
    vector<pair<int, long>> tmpHashMap;
    for (ConditionalProbabilityFunction* cpf : task->CPFs) {
        if (eval->dependentStateFluents.find(cpf->head) !=
            eval->dependentStateFluents.end()) {
            tmpHashMap.push_back(make_pair(cpf->head->index, hashKeyBase));
            if (!utils::MathUtils::multiplyWithOverflowCheck(
                hashKeyBase, cpf->getDomainSize())) {
                eval->cachingType = "NONE";
                return;
            }
        }
    }

    // We store the information the other way around, i.e. we add eval and the
    // hash key base to each state variable eval depends on
    vector<vector<pair<int, long>>>& hashMap =
        task->indexToStateFluentHashKeyMap;
    for (unsigned int index = 0; index < tmpHashMap.size(); ++index) {
        hashMap[tmpHashMap[index].first].push_back(
            make_pair(eval->hashIndex, tmpHashMap[index].second));
        eval->stateFluentHashKeyBases.push_back(tmpHashMap[index]);
    }

    // At this point, hashKeyBase is equal to the number of perfect hash keys
    // of eval. Depending on that number, we use a vector or a map for caching.
    // TODO: Make sure this arbitrarily chosen constant makes sense
    if (hashKeyBase > 1000000) {
        eval->cachingType = "MAP";
    } else {
        eval->cachingType = "VECTOR";
        eval->precomputedResults.resize(hashKeyBase,
                                        -numeric_limits<double>::max());
        if (eval->isProbabilistic()) {
            eval->precomputedPDResults.resize(hashKeyBase);
        }
    }
}

void HashKeyGenerator::prepareKleeneStateFluentHashKeys(Evaluatable* eval,
                                                        long hashKeyBase) {
    // tmpHashMap[i].first is the index of a state variable that affects eval,
    // and the hash key base of that state variable is tmpHashMap[i].second
    vector<pair<int, long>> tmpHashMap;
    for (ConditionalProbabilityFunction* cpf : task->CPFs) {
        if (eval->dependentStateFluents.find(cpf->head) !=
            eval->dependentStateFluents.end()) {
            tmpHashMap.push_back(make_pair(cpf->head->index, hashKeyBase));
            if ((cpf->kleeneDomainSize == 0) ||
                !utils::MathUtils::multiplyWithOverflowCheck(
                    hashKeyBase, cpf->kleeneDomainSize)) {
                eval->kleeneCachingType = "NONE";
                return;
            }
        }
    }

    // We store the information the other way around, i.e. we add eval and the
    // hash key base to each state variable eval depends on
    vector<vector<pair<int, long>>>& hashMap =
        task->indexToKleeneStateFluentHashKeyMap;
    for (unsigned int index = 0; index < tmpHashMap.size(); ++index) {
        hashMap[tmpHashMap[index].first].push_back(
            make_pair(eval->hashIndex, tmpHashMap[index].second));
    }

    // At this point, hashKeyBase is equal to the number of perfect hash keys
    // of eval. Depending on that number, we use a vector or a map for caching.
    // TODO: Make sure this arbitrarily chosen constant makes sense
    if (hashKeyBase > 200000) {
        eval->kleeneCachingType = "MAP";
    } else {
        eval->kleeneCachingType = "VECTOR";
        eval->kleeneCachingVectorSize = hashKeyBase;
    }
}
} // namespace prost::parser::hashing
