#include "mc_uct_search.h"

using namespace std;

/******************************************************************
                        Initialization
******************************************************************/

void MCUCTSearch::initializeDecisionNodeChild(MCUCTNode* node, unsigned int const& actionIndex, double const& initialQValue) {
    node->children[actionIndex] = getSearchNode();
    node->children[actionIndex]->futureReward = (double)numberOfInitialVisits * (double)remainingConsideredSteps() * initialQValue;
    node->children[actionIndex]->numberOfVisits = numberOfInitialVisits;

    node->numberOfVisits += numberOfInitialVisits;
    node->futureReward = std::max(node->futureReward, node->children[actionIndex]->futureReward);

    // cout << "initializing child ";
    // SearchEngine::printAction(cout,index);
    // cout << " with " << initialQValue << " leading to init of " << node->children[index]->accumulatedReward << endl;
}

/******************************************************************
                         Outcome selection
******************************************************************/

MCUCTNode* MCUCTSearch::selectOutcome(MCUCTNode* node, PDState& nextPDState, State& nextState, int& varIndex) {
    // TODO: No node should be created if nextPDState[varIndex] is deterministic
    if(node->children.empty()) {
        node->children.resize(SearchEngine::probCPFs[varIndex]->getDomainSize(), NULL);
    }

    int childIndex = (int)sampleVariable(nextPDState[varIndex]);
    nextState[varIndex] = childIndex;

    if(!node->children[childIndex]) {
        node->children[childIndex] = getSearchNode();
    }
    return node->children[childIndex];
}

/******************************************************************
                          Backup functions
******************************************************************/

void MCUCTSearch::backupDecisionNode(MCUCTNode* node, double const& immReward, double const& futReward) {
    node->immediateReward += immReward;
    node->futureReward += futReward;

    ++node->numberOfVisits;
}

void MCUCTSearch::backupChanceNode(MCUCTNode* node, double const& futReward) {
    assert(MathUtils::doubleIsEqual(node->immediateReward, 0.0));

    node->futureReward += futReward;
    ++node->numberOfVisits;
}



