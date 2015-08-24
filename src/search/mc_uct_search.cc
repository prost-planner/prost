#include "mc_uct_search.h"

using namespace std;

/******************************************************************
                         Outcome selection
******************************************************************/

MCUCTNode* MCUCTSearch::selectOutcome(MCUCTNode* node, 
                                      PDState& nextState,
                                      int const& varIndex,
                                      int const& lastProbVarIndex) {
    if (node->children.empty()) {
        node->children.resize(
                SearchEngine::probabilisticCPFs[varIndex]->getDomainSize(),
                NULL);
    }

    int childIndex = (int)nextState.sample(varIndex);

    if (!node->children[childIndex]) {
        if (varIndex == lastProbVarIndex) {
            node->children[childIndex] = getDecisionNode(1.0);
        } else {
            node->children[childIndex] = getChanceNode(1.0);
        }
    }

    return node->children[childIndex];
}

/******************************************************************
                          Backup functions
******************************************************************/

void MCUCTSearch::backupDecisionNode(MCUCTNode* node, double const& futReward) {
    if (MathUtils::doubleIsMinusInfinity(node->futureReward)) {
        node->futureReward = futReward;
    } else {
        node->futureReward += futReward;
    }

    ++node->numberOfVisits;
}

void MCUCTSearch::backupChanceNode(MCUCTNode* node, double const& futReward) {
    assert(MathUtils::doubleIsEqual(node->immediateReward, 0.0));

    node->futureReward += futReward;
    ++node->numberOfVisits;
}
