#include "max_mc_uct_search.h"

using namespace std;

/******************************************************************
                         Outcome selection
******************************************************************/

THTSSearchNode* MaxMCUCTSearch::selectOutcome(THTSSearchNode* node,
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

void MaxMCUCTSearch::backupDecisionNodeLeaf(THTSSearchNode* node,
                                            double const& futReward) {
    node->futureReward = futReward;

    ++node->numberOfVisits;

    // cout << "updated dec node leaf with immediate reward " << immReward << endl;
    // node->print(cout);
    // cout << endl;
}

void MaxMCUCTSearch::backupDecisionNode(THTSSearchNode* node,
                                        double const& /*futReward*/) {
    assert(!node->children.empty());

    ++node->numberOfVisits;

    // Propagate values from best child
    node->futureReward = -numeric_limits<double>::max();
    for (unsigned int childIndex = 0; childIndex < node->children.size();
         ++childIndex) {
        if (node->children[childIndex]) {
            node->futureReward =
                std::max(node->futureReward,
                         node->children[childIndex]->getExpectedRewardEstimate());
        }
    }

    // cout << "updated dec node with immediate reward " << immReward << endl;
    // node->print(cout);
    // cout << endl;
}

void MaxMCUCTSearch::backupChanceNode(THTSSearchNode* node,
                                      double const& /*futReward*/) {
    assert(MathUtils::doubleIsEqual(node->immediateReward, 0.0));

    ++node->numberOfVisits;
    node->futureReward = 0.0;
    int numberOfChildVisits = 0;

    // Propagate values from children
    for (unsigned int i = 0; i < node->children.size(); ++i) {
        if (node->children[i]) {
            node->futureReward +=
                (node->children[i]->numberOfVisits *
                 node->children[i]->getExpectedRewardEstimate());
            numberOfChildVisits += node->children[i]->numberOfVisits;
        }
    }

    node->futureReward /= numberOfChildVisits;
}
