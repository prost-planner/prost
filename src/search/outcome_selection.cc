#include "outcome_selection.h"

#include "thts.h"

SearchNode* MCOutcomeSelection::selectOutcome(SearchNode* node,
                                              PDState& nextState,
                                              int const& varIndex,
                                              int const& lastProbVarIndex) {

    if (node->children.empty()) {
        node->children.resize(
            SearchEngine::probabilisticCPFs[varIndex]->getDomainSize(), nullptr);
    }

    int childIndex = (int)nextState.sample(varIndex);

    if (!node->children[childIndex]) {
        if (varIndex == lastProbVarIndex) {
            node->children[childIndex] = thts->getDecisionNode(1.0);
        } else {
            node->children[childIndex] = thts->getChanceNode(1.0);
        }
    }

    return node->children[childIndex];
}

SearchNode* UnsolvedMCOutcomeSelection::selectOutcome(
        SearchNode* node, PDState& nextState, int const& varIndex, int const& lastProbVarIndex) {
    DiscretePD& pd = nextState.probabilisticStateFluentAsPD(varIndex);
    assert(pd.isWellDefined());

    double probSum = 1.0;
    int childIndex = 0;

    if (node->children.empty()) {
        node->children.resize(
            SearchEngine::probabilisticCPFs[varIndex]->getDomainSize(), nullptr);
    } else {
        // Determine the sum of the probabilities of unsolved outcomes
        for (unsigned int i = 0; i < pd.size(); ++i) {
            childIndex = pd.values[i];
            if (node->children[childIndex] &&
                node->children[childIndex]->solved) {
                probSum -= pd.probabilities[i];
            }
        }
    }

    assert(MathUtils::doubleIsGreater(probSum, 0.0) && 
           MathUtils::doubleIsSmallerOrEqual(probSum, 1.0));

    double randNum = MathUtils::generateRandomNumber() * probSum;
    //cout << "ProbSum is " << probSum << endl;
    //cout << "RandNum is " << randNum << endl;

    probSum = 0.0;
    double childProb = 0.0;

    for (unsigned int i = 0; i < pd.size(); ++i) {
        childIndex = pd.values[i];
        if (!node->children[childIndex] ||
            !node->children[childIndex]->solved) {
            probSum += pd.probabilities[i];
            if (MathUtils::doubleIsSmaller(randNum, probSum)) {
                childProb = pd.probabilities[i];
                break;
            }
        }
    }

    // cout << "Chosen child is " << childIndex << " and prob is " << childProb << endl;

    assert((childIndex >= 0) && childIndex < node->children.size());

    if (!node->children[childIndex]) {
        if (varIndex == lastProbVarIndex) {
            node->children[childIndex] = thts->getDecisionNode(childProb);
        } else {
            node->children[childIndex] = thts->getChanceNode(childProb);
        }
    }

    assert(!node->children[childIndex]->solved);

    nextState.probabilisticStateFluent(varIndex) = childIndex;
    return node->children[childIndex];
}

