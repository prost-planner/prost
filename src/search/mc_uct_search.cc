#include "mc_uct_search.h"

using namespace std;

/******************************************************************
                         Outcome selection
******************************************************************/

THTSSearchNode* MCUCTSearch::selectOutcome(THTSSearchNode* node, 
                                           PDState& nextState,
                                           int const& varIndex,
                                           int const& lastProbVarIndex) {
    if (node->children.empty()) {
        node->children.resize(
            SearchEngine::probabilisticCPFs[varIndex]->getDomainSize(), NULL);
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

void MCUCTSearch::backupDecisionNodeLeaf(THTSSearchNode* node,
                                         double const& futReward) {
    ++node->numberOfVisits;

    node->futureReward = futReward;
}

void MCUCTSearch::backupDecisionNode(THTSSearchNode* node, double const& /*futReward*/) {
    ++node->numberOfVisits;

    node->futureReward = -std::numeric_limits<double>::max();
    for (THTSSearchNode* child : node->children) {
        if (child) {
            node->futureReward =
                std::max(node->futureReward, child->getExpectedRewardEstimate());
        }
    }
}

void MCUCTSearch::backupChanceNode(THTSSearchNode* node, double const& futReward) {
    ++node->numberOfVisits;

    node->futureReward = node->futureReward +
        initialLearningRate * (futReward - node->futureReward) / 
        (1.0 + (learningRateDecay * (double)node->numberOfVisits));
}
