#include "hash_key_generator.h"

#include "evaluatables.h"
#include "rddl.h"

#include "utils/math_utils.h"
#include "utils/timer.h"

#include <iostream>

using namespace std;

namespace prost {
namespace parser {
void HashKeyGenerator::generateHashKeys(bool output) {
    utils::Timer t;
    // Initialize state hash keys
    if (output) {
        cout << "    Preparing state hash keys..." << endl;
    }
    prepareStateHashKeys();
    if (output) {
        cout << "    ...finished (" << t() << ")" << endl;
    }
    t.reset();

    // Initialize Kleene state hash keys
    if (output) {
        cout << "    Preparing Kleene state hash keys..." << endl;
    }
    prepareKleeneStateHashKeys();
    if (output) {
        cout << "    ...finished (" << t() << ")" << endl;
    }
    t.reset();

    // Initialize hash keys of evaluatables
    if (output) {
        cout << "    Preparing hash keys of evaluatables..." << endl;
    }
    prepareEvaluatableHashKeys();
    if (output) {
        cout << "    ...finished (" << t() << ")" << endl;
    }
}


// Check if hashing of states is possible, and assign hash key bases if so
void HashKeyGenerator::prepareStateHashKeys() {
    bool stateHashingPossible = true;
    long nextHashKeyBase = 1;

    for (ConditionalProbabilityFunction* cpf : task->CPFs) {
        if (!cpf->hasFiniteDomain()) {
            stateHashingPossible = false;
            break;
        }
        int numValues = cpf->getDomainSize();
        vector<long> stateHashKeys(numValues);
        for (size_t value = 0; value < numValues; ++value) {
            stateHashKeys[value] = value * nextHashKeyBase;
        }

        if (!utils::MathUtils::multiplyWithOverflowCheck(
            nextHashKeyBase, numValues)) {
            stateHashingPossible = false;
            break;
        }
        task->stateHashKeys.push_back(stateHashKeys);
    }

    if (!stateHashingPossible) {
        task->stateHashKeys.clear();
    }
}

// Check if hashing of KleeneStates is possible, and assign hash key bases if so
void HashKeyGenerator::prepareKleeneStateHashKeys() {
    bool kleeneStateHashingPossible = true;

    // We start by calculating the kleene domain size
    for (ConditionalProbabilityFunction* cpf : task->CPFs) {
        if (!cpf->hasFiniteDomain()) {
            kleeneStateHashingPossible = false;
            cpf->kleeneDomainSize = 0;
            continue;
        }
        int numValues = cpf->getDomainSize();

        // The Kleene domain size is 2^n-1, where n is the number of values of
        // this CPF
        cpf->kleeneDomainSize = 2;
        if (utils::MathUtils::toThePowerOfWithOverflowCheck(
            cpf->kleeneDomainSize, numValues)) {
            --cpf->kleeneDomainSize;
        } else {
            // KleeneState hashing is impossible because the Kleene domain of
            // this CPF is too large i.e., the computation of 2^n leads to an
            // overflow). We nevertheless continue the computation as the
            // Kleene domains can still be used for Kleene hash keys of
            // evaluatables.
            kleeneStateHashingPossible = false;
            cpf->kleeneDomainSize = 0;
        }
    }

    if (!kleeneStateHashingPossible) {
        return;
    }

    long nextHashKeyBase = 1;
    for (ConditionalProbabilityFunction* cpf : task->CPFs) {
        task->kleeneStateHashKeyBases.push_back(nextHashKeyBase);

        if (!utils::MathUtils::multiplyWithOverflowCheck(
            nextHashKeyBase, cpf->kleeneDomainSize)) {
            kleeneStateHashingPossible = false;
            break;
        }
    }

    if (!kleeneStateHashingPossible) {
        task->kleeneStateHashKeyBases.clear();
    }
}

void HashKeyGenerator::prepareEvaluatableHashKeys() {
    int numCPFs = task->CPFs.size();
    task->indexToStateFluentHashKeyMap.resize(numCPFs);
    task->indexToKleeneStateFluentHashKeyMap.resize(numCPFs);

    int hashIndex = 0;
    for (ConditionalProbabilityFunction* cpf : task->CPFs) {
        cpf->hashIndex = hashIndex;
        cpf->initializeHashKeys(task);
        ++hashIndex;
    }
    task->rewardCPF->hashIndex = hashIndex;
    task->rewardCPF->initializeHashKeys(task);

    for (ActionPrecondition* precond : task->preconds) {
        ++hashIndex;
        precond->hashIndex = hashIndex;
        precond->initializeHashKeys(task);
    }
}
} // namespace parser
} // namespace prost
