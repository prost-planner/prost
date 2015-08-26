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

void MCUCTSearch::backupDecisionNodeLeaf(MCUCTNode* node,
                                         double const& futReward) {
    ++node->numberOfVisits;

    node->futureReward = futReward;
}

void MCUCTSearch::backupDecisionNode(MCUCTNode* node, double const& /*futReward*/) {
    ++node->numberOfVisits;

    node->futureReward = -std::numeric_limits<double>::max();
    for (MCUCTNode* child : node->children) {
        if (child) {
            node->futureReward =
                std::max(node->futureReward, child->getExpectedRewardEstimate());
        }
    }
}

void MCUCTSearch::backupChanceNode(MCUCTNode* node, double const& futReward) {
    ++node->numberOfVisits;

    node->futureReward = node->futureReward +
        initialLearningRate * (futReward - node->futureReward) / 
        (1.0 + (learningRateDecay * (double)node->numberOfVisits));
}
