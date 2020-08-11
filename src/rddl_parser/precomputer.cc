#include "precomputer.h"

#include "evaluatables.h"
#include "rddl.h"

#include "utils/math.h"

#include <set>

using namespace std;

namespace prost::parser {
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
    vector<StateFluent*> dependentStateFluents(
        eval->dependentStateFluents.begin(), eval->dependentStateFluents.end());

    vector<State> relevantStates;
    if (!dependentStateFluents.empty()) {
        createRelevantStates(dependentStateFluents, relevantStates);
    } else {
        relevantStates.emplace_back(task->CPFs.size());
    }

    for (State const& state : relevantStates) {
        long hashKey = calculateStateFluentHashKey(eval, state);
        set<long> usedActionHashKeys;
        for (ActionState const& action : task->actionStates) {
            long actionHashKey = eval->actionHashKeyMap[action.index];
            if (usedActionHashKeys.find(actionHashKey) ==
                usedActionHashKeys.end()) {
                usedActionHashKeys.insert(actionHashKey);

                double& res = eval->precomputedResults[hashKey + actionHashKey];
                assert(utils::doubleIsMinusInfinity(res));
                if (eval->isProbabilistic()) {
                    eval->determinization->evaluate(res, state, action);
                    DiscretePD& pdRes =
                        eval->precomputedPDResults[hashKey + actionHashKey];
                    assert(pdRes.isUndefined());
                    eval->formula->evaluateToPD(pdRes, state, action);
                } else {
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

    size_t domainSize = task->CPFs[fluent->index]->domain.size();
    if (result.empty()) {
        for (int value = 0; value < domainSize; ++value) {
            State base(task->CPFs.size());
            base[fluent->index] = value;
            result.push_back(base);
        }
    } else {
        int size = result.size();
        for (int stateIndex = 0; stateIndex < size; ++stateIndex) {
            // result already contains a state where the fluent has value '0',
            // so we only have to generate states for the remaining values
            for (unsigned int value = 1; value < domainSize; ++value) {
                State const& state = result[stateIndex];
                assert(state[fluent->index] == 0);
                result.emplace_back(state, fluent->index, value);
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
        long hashKeyBase = hashKeyBasePair.second;
        res += value * hashKeyBase;
    }
    return res;
}
} // namespace prost::parser
