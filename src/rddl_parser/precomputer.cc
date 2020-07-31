#include "precomputer.h"

#include "evaluatables.h"
#include "rddl.h"

#include "utils/math_utils.h"

#include <set>

using namespace std;

void Precomputer::precompute() {
    for (ConditionalProbabilityFunction* cpf : task->CPFs) {
        if (cpf->cachingType == "VECTOR") {
            precomputeEvaluatable(cpf);
        }
    }

    if (task->rewardCPF->cachingType == "VECTOR") {
        precomputeEvaluatable(task->rewardCPF);
    }

    for (ActionPrecondition* precond : task->preconds) {
        if (precond->cachingType == "VECTOR") {
            precomputeEvaluatable(precond);
        }
    }
}

void Precomputer::precomputeEvaluatable(Evaluatable* eval) {
    int numCPFs = task->CPFs.size();
    vector<StateFluent*> dependentStateFluents;
    for (StateFluent* sf : eval->dependentStateFluents) {
        dependentStateFluents.push_back(sf);
    }

    vector<State> relevantStates;
    if (!dependentStateFluents.empty()) {
        createRelevantStates(dependentStateFluents, relevantStates);
    } else {
        relevantStates.emplace_back(numCPFs);
    }

    for (State const& state : relevantStates) {
        long hashKey = calculateStateFluentHashKey(eval, state);
        set<long> usedActionHashKeys;
        for (ActionState const& action : task->actionStates) {
            long actionHashKey = eval->actionHashKeyMap[action.index];
            if (usedActionHashKeys.find(actionHashKey) ==
                usedActionHashKeys.end()) {
                usedActionHashKeys.insert(actionHashKey);

                if (eval->isProbabilistic()) {
                    double& res =
                        eval->precomputedResults[hashKey + actionHashKey];
                    assert(MathUtils::doubleIsMinusInfinity(res));
                    eval->determinization->evaluate(res, state, action);

                    DiscretePD& pdRes =
                        eval->precomputedPDResults[hashKey + actionHashKey];
                    assert(pdRes.isUndefined());
                    eval->formula->evaluateToPD(pdRes, state, action);
                } else {
                    double& res =
                        eval->precomputedResults[hashKey + actionHashKey];
                    assert(MathUtils::doubleIsMinusInfinity(res));
                    eval->formula->evaluate(res, state, action);
                }
            }
        }
    }
}

void Precomputer::createRelevantStates(
    vector<StateFluent*>& dependentStateFluents, vector<State>& result) {
    StateFluent* fluent = dependentStateFluents.back();
    dependentStateFluents.pop_back();

    int fluentIndex = fluent->index;
    int domainSize = task->CPFs[fluentIndex]->domain.size();

    if (result.empty()) {
        int numCPFs = task->CPFs.size();
        for (int value = 0; value < domainSize; ++value) {
            State base(numCPFs);
            base[fluentIndex] = value;
            result.push_back(base);
        }
    } else {
        int size = result.size();
        for (int stateIndex = 0; stateIndex < size; ++stateIndex) {
            // The state with value '0' is already in the set, so we generate
            // only all other states
            for (unsigned int value = 1; value < domainSize; ++value) {
                State const& state = result[stateIndex];
                assert(state[fluentIndex] == 0);
                result.emplace_back(state, fluentIndex, value);
            }
        }
    }

    if (!dependentStateFluents.empty()) {
        createRelevantStates(dependentStateFluents, result);
    }
}

long Precomputer::calculateStateFluentHashKey(Evaluatable* eval,
                                              State const& state) const {
    long res = 0;
    vector<pair<int, long>> const& hashKeyBases = eval->stateFluentHashKeyBases;
    for (pair<int, long> const& hashKeyBasePair : hashKeyBases) {
        int value = state[hashKeyBasePair.first];
        int hashKeyBase = hashKeyBasePair.second;
        res += value * hashKeyBase;
    }
    return res;
}
