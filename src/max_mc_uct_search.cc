#include "max_mc_uct_search.h"

using namespace std;

/******************************************************************
                        Initialization
******************************************************************/

void MaxMCUCTSearch::initializeDecisionNodeChild(MaxMCUCTNode* node, unsigned int const& actionIndex, double const& initialQValue) {
    node->children[actionIndex] = getSearchNode();
    node->children[actionIndex]->futureReward = heuristicWeight * (double)remainingConsideredSteps() * initialQValue;
    node->children[actionIndex]->numberOfVisits = numberOfInitialVisits;

    node->numberOfVisits += numberOfInitialVisits;
    node->futureReward = std::max(node->futureReward, node->children[actionIndex]->getExpectedRewardEstimate());

    // cout << "initializing child ";
    // successorGenerator->printAction(cout,index);
    // cout << " with " << initialQValue << " leading to init of " << node->children[index]->accumulatedReward << endl;
}

/******************************************************************
                         Outcome selection
******************************************************************/

MaxMCUCTNode* MaxMCUCTSearch::selectOutcome(MaxMCUCTNode* node, PDState& nextPDState, State& nextState, int& varIndex) {
    // TODO: No node should be created if nextPDState[varIndex] is deterministic
    if(node->children.empty()) {
        node->children.resize(successorGenerator->getDomainSizeOfCPF(varIndex), NULL);
    }

    int childIndex = (int)successorGenerator->sampleVariable(nextPDState[varIndex]);
    nextState[varIndex] = childIndex;

    if(!node->children[childIndex]) {
        node->children[childIndex] = getSearchNode();
    }
    return node->children[childIndex];
}

/******************************************************************
                          Backup functions
******************************************************************/

void MaxMCUCTSearch::backupDecisionNodeLeaf(MaxMCUCTNode* node, double const& immReward, double const& futReward) {
    node->immediateReward = immReward;
    node->futureReward = futReward;

    ++node->numberOfVisits;

    // cout << "updated dec node leaf with immediate reward " << immReward << endl;
    // node->print(cout);
    // cout << endl;
}

void MaxMCUCTSearch::backupDecisionNode(MaxMCUCTNode* node, double const& immReward, double const& /*futReward*/) {
    // TODO: Store the best child. Then:

    // 1. The best child has been sampled. If futReward is at least as
    // good as that child's reward before it is still the best child
    // (it got better). Only otherwise check for the best child again.

    // 2. Another child has been sampled. Check if it is better than
    // the former best child. If so, it is the new best child,
    // otherwise the former best child is still the best child.

    // Identify best child and take its reward estimate
    if(selectedActionIndex() != -1) {
        ++node->numberOfVisits;
    }

    node->immediateReward = immReward;
    node->futureReward = node->children[0]->getExpectedRewardEstimate();

    for(unsigned int childIndex = 1; childIndex < node->children.size(); ++childIndex) {
        if(node->children[childIndex]) {
            node->futureReward = std::max(node->futureReward, node->children[childIndex]->getExpectedRewardEstimate());
        }
    }

    // cout << "updated dec node with immediate reward " << immReward << endl;
    // node->print(cout);
    // cout << endl;
}

void MaxMCUCTSearch::backupChanceNode(MaxMCUCTNode* node, double const& /*futReward*/) {
    assert(MathUtils::doubleIsEqual(node->immediateReward, 0.0));

    ++node->numberOfVisits;
    node->futureReward = 0.0;
    int numberOfChildVisits = 0;

    // Propagate values from children
    for(unsigned int i = 0; i < node->children.size(); ++i) {
        node->futureReward += (node->children[i]->numberOfVisits * node->children[i]->getExpectedRewardEstimate());
        numberOfChildVisits += node->children[i]->numberOfVisits;
    }

    node->futureReward /= numberOfChildVisits;
}



