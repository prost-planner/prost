#include "max_mc_uct_search.h"

using namespace std;

/******************************************************************
                        Initialization
******************************************************************/

void MaxMCUCTSearch::initializeDecisionNodeChild(
        MaxMCUCTNode* node, unsigned int const& actionIndex,
        double const& initialQValue) {
    node->children[actionIndex] = getSearchNode();
    node->children[actionIndex]->futureReward = heuristicWeight *
                                                (double)
                                                remainingConsideredSteps() *
                                                initialQValue;
    node->children[actionIndex]->numberOfVisits = numberOfInitialVisits;

    node->numberOfVisits += numberOfInitialVisits;
    node->futureReward =
        std::max(node->futureReward,
                node->children[actionIndex]->getExpectedRewardEstimate());

    // cout << "initialized child ";
    // SearchEngine::actionStates[actionIndex].printCompact(cout);
    // cout << " with remaining steps " << remainingConsideredSteps() << " and initialQValue " << initialQValue << endl;
    // node->children[actionIndex]->print(cout);
    // cout << endl;
}

/******************************************************************
                         Outcome selection
******************************************************************/

MaxMCUCTNode* MaxMCUCTSearch::selectOutcome(MaxMCUCTNode* node,
        PDState& nextState,
        int& varIndex) {
    // TODO: No node should be created if nextPDState[varIndex] is deterministic
    if (node->children.empty()) {
        node->children.resize(
                SearchEngine::probabilisticCPFs[varIndex]->getDomainSize(),
                NULL);
    }

    int childIndex = (int) sampleVariable(
            nextState.probabilisticStateFluentAsPD(varIndex));
    nextState.probabilisticStateFluent(varIndex) = childIndex;

    if (!node->children[childIndex]) {
        node->children[childIndex] = getSearchNode();
    }
    return node->children[childIndex];
}

/******************************************************************
                          Backup functions
******************************************************************/

void MaxMCUCTSearch::backupDecisionNodeLeaf(MaxMCUCTNode* node,
        double const& immReward,
        double const& futReward) {
    node->immediateReward = immReward;
    node->futureReward = futReward;

    ++node->numberOfVisits;

    // cout << "updated dec node leaf with immediate reward " << immReward << endl;
    // node->print(cout);
    // cout << endl;
}

void MaxMCUCTSearch::backupDecisionNode(MaxMCUCTNode* node,
        double const& immReward,
        double const& /*futReward*/) {
    assert(!node->children.empty());

    node->immediateReward = immReward;

    if (selectedActionIndex() != -1) {
        ++node->numberOfVisits;
    }

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

void MaxMCUCTSearch::backupChanceNode(MaxMCUCTNode* node,
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
